#include "DotsPlayer.h"
#include <mutex>
#include <string>
#include <unordered_set>
#include <sys/socket.h>
#include <unistd.h>
#include <dots.h>
#include "Networking/data.h"
#include "Networking/Player.h"
#include "Networking/sockets.h"

unordered_set<int> DotsPlayer::curTags;
mutex DotsPlayer::curTagsLock;

static int djb2_hash(const string& str) {
    unsigned int hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash & ((unsigned int) -1 >> 1);
}

DotsPlayer::DotsPlayer(const string& id) :
        Player(Names(dots_world_rank, dots_world_size)), id(id) {
    dotsTag = djb2_hash(id);
    {
        lock_guard<mutex> lock(curTagsLock);
        if (curTags.find(dotsTag) != curTags.end()) {
            throw runtime_error("Hash collision in dotsTag; cannot proceed");
        }
        curTags.insert(dotsTag);
    }
}

int DotsPlayer::num_players() const {
    return dots_world_size;
}

int DotsPlayer::my_num() const {
    return dots_world_rank;
}

void DotsPlayer::send_to_no_stats(int player, const octetStream& o) const {
    octet lenbuf[LENGTH_SIZE];
    encode_length(lenbuf, o.get_length(), LENGTH_SIZE);

#ifdef VERBOSE_COMM
    cout << id << ": Sending " << o.get_length() << " bytes to " << player << endl;
#endif

    if (dots_msg_send(lenbuf, sizeof(lenbuf), player, dotsTag)) {
        throw runtime_error("Error in dots_msg_send");
    }
    if (dots_msg_send(o.get_data(), o.get_length(), player, dotsTag)) {
        throw runtime_error("Error in dots_msg_send");
    }

#ifdef VERBOSE_COMM
    cout << id << ": Sent " << o.get_length() << " bytes to " << player << endl;
#endif
}

void DotsPlayer::receive_player_no_stats(int player, octetStream& o) const {
    octet lenbuf[LENGTH_SIZE];

#ifdef VERBOSE_COMM
    cout << id << ": Receiving from " << player << endl;
#endif

    size_t bytes_received;
    if (dots_msg_recv(lenbuf, sizeof(lenbuf), player, dotsTag,
                &bytes_received)) {
        throw runtime_error("Error in dots_msg_recv");
    }
    if (bytes_received < sizeof(lenbuf)) {
        throw runtime_error("Not enough bytes received in dots_msg_recv");
    }

    size_t len = decode_length(lenbuf, LENGTH_SIZE);
    o.reset_write_head();
    o.resize_min(len);
    octet *ptr = o.append(len);

    if (dots_msg_recv(ptr, len, player, dotsTag, &bytes_received)) {
        throw runtime_error("Error in dots_msg_recv");
    }
    if (bytes_received < len) {
        throw runtime_error("Not enough bytes received in dots_msg_recv");
    }

#ifdef VERBOSE_COMM
    cout << id << ": Received " << o.get_length() << " bytes from " << player << endl;
#endif
}

size_t DotsPlayer::send_no_stats(int player, const PlayerBuffer& buffer,
        bool block __attribute__((unused))) const {

#ifdef VERBOSE_COMM
    cout << id << ": [Player buffer] Sending " << buffer.size << " bytes to " << player << endl;
#endif

    if (dots_msg_send(buffer.data, buffer.size, player, dotsTag)) {
        throw runtime_error("Error in dots_msg_send");
    }

#ifdef VERBOSE_COMM
    cout << id << ": [Player buffer] Sent " << buffer.size << " bytes to " << player << endl;
#endif

    return buffer.size;
}

size_t DotsPlayer::recv_no_stats(int player, const PlayerBuffer& buffer,
        bool block __attribute__((unused))) const {

#ifdef VERBOSE_COMM
    cout << id << ": [Player buffer] Receiving " << buffer.size << " bytes from " << player << endl;
#endif

    size_t bytes_received;
    if (dots_msg_recv(buffer.data, buffer.size, player, dotsTag,
                &bytes_received)) {
        throw runtime_error("Error in dots_msg_recv");
    }
    if (bytes_received < buffer.size) {
        throw runtime_error("Not enough bytes received in dots_msg_recv");
    }

#ifdef VERBOSE_COMM
    cout << id << ": [Player buffer] Received " << buffer.size << " bytes from " << player << endl;
#endif

    return buffer.size;
}

void DotsPlayer::exchange_no_stats(int other, const octetStream& to_send,
        octetStream& to_receive) const {
    send_to_no_stats(other, to_send);
    receive_player_no_stats(other, to_receive);
}

void DotsPlayer::pass_around_no_stats(const octetStream& to_send,
      octetStream& to_receive, int offset) const {
    send_to_no_stats(get_player(offset), to_send);
    receive_player_no_stats(get_player(-offset), to_receive);
}

void DotsPlayer::Broadcast_Receive_no_stats(vector<octetStream>& o) const {
    for (int i = 0; i < num_players(); i++) {
        if (i == my_num()) {
            continue;
        }
        send_to_no_stats(i, o[my_num()]);
    }
    for (int i = 0; i < num_players(); i++) {
        if (i == my_num()) {
            continue;
        }
        receive_player_no_stats(i, o[i]);
    }
}

void DotsPlayer::send_receive_all_no_stats(const vector<vector<bool>>& channels,
        const vector<octetStream>& to_send,
        vector<octetStream>& to_receive) const {
    for (int i = 0; i < num_players(); i++) {
        if (i == my_num()) {
            continue;
        }
        if (channels[my_num()][i]) {
            send_to_no_stats(i, to_send[i]);
        }
    }
    for (int i = 0; i < num_players(); i++) {
        if (i == my_num()) {
            continue;
        }
        to_receive.resize(num_players());
        if (channels[i][my_num()]) {
            receive_player_no_stats(i, to_receive[i]);
        }
    }
}

DotsPlayer::~DotsPlayer() {}
