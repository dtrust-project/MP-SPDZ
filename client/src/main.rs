use dotspb::dec_exec::dec_exec_client::DecExecClient;
use dotspb::dec_exec::App;
use futures::future;
use tonic::Request;
use uuid::Uuid;

const APP_NAME: &str = "mpspdz";

fn uuid_to_uuidpb(id: Uuid) -> dotspb::dec_exec::Uuid {
    dotspb::dec_exec::Uuid {
        hi: (id.as_u128() >> 64) as u64,
        lo: id.as_u128() as u64,
    }
}

#[tokio::main]
async fn main() {
    let node_addrs = [
        "http://localhost:50050",
        "http://localhost:50051",
        "http://localhost:50052",
        "http://localhost:50053",
    ];
    let mut clients = future::join_all(
            node_addrs
                .iter()
                .map(|addr| DecExecClient::connect(addr.to_owned()))
        )
        .await
        .into_iter()
        .collect::<Result<Vec<_>, _>>()
        .unwrap();

    let request_id = Uuid::new_v4();
    let res = future::join_all(
            clients.iter_mut()
                .map(|client| client.exec(Request::new(App {
                    app_name: APP_NAME.to_owned(),
                    app_uid: 0,
                    request_id: Some(uuid_to_uuidpb(request_id)),
                    client_id: "".to_owned(),
                    func_name: "unused".to_owned(),
                    in_files: vec!["input".to_owned()],
                    out_files: vec!["output".to_owned()],
                    args: vec![],
                })))
        )
        .await
        .into_iter()
        .collect::<Result<Vec<_>, _>>()
        .unwrap()
        .into_iter()
        .map(|res| res.into_inner())
        .collect::<Vec<_>>();

    println!("{res:?}");
}
