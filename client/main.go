package main

import (
	"context"
	"encoding/binary"
	"flag"
	"fmt"

	"github.com/dtrust-project/dotspb/go/dotspb"
	"github.com/google/uuid"
	"google.golang.org/grpc"
)

const appName = "mpspdz"

func uuidToPb(id uuid.UUID) *dotspb.UUID {
	idBytes, err := id.MarshalBinary()
	if err != nil {
		panic(err)
	}
	return &dotspb.UUID{
		Hi: binary.BigEndian.Uint64(idBytes[:8]),
		Lo: binary.BigEndian.Uint64(idBytes[8:]),
	}
}

func main() {
	var numServers int
	flag.IntVar(&numServers, "num", 0, "Number of servers to connect to")
	flag.Parse()

	if numServers == 0 {
		flag.Usage()
		return
	}

	clients := make([]dotspb.DecExecClient, numServers)
	for i := 0; i < numServers; i++ {
		conn, err := grpc.Dial(fmt.Sprintf("localhost:%d", 50050+i), grpc.WithInsecure())
		if err != nil {
			panic(err)
		}
		clients[i] = dotspb.NewDecExecClient(conn)
	}

	resChan := make(chan *dotspb.Result)
	errChan := make(chan error)
	ctx := context.Background()
	ctx, cancel := context.WithCancel(ctx)
	requestId := uuid.New()
	for _, client := range clients {
		client := client
		go func() {
			res, err := client.Exec(ctx, &dotspb.App{
				AppName: appName,
				FuncName: "unused",
				RequestId: uuidToPb(requestId),
				InFiles: []string{"input"},
				OutFiles: []string{"output"},
			})
			if err != nil {
				errChan <- err
				return
			}
			resChan <- res
		}()
	}

	for range clients {
		select {
		case res := <-resChan:
			fmt.Println(res)
		case err := <-errChan:
			cancel()
			panic(err)
		}
	}
}
