#include "ps/ps.h"
using namespace ps;

class KVStoreDistServer {
public:
  KVStoreDistServer(bool sync = true) {
    using namespace std::placeholders;
    ps_server_ = new KVServer<float>(0);
    sync_ = sync;
    ps_server_->set_request_handle(
      std::bind(&KVStoreDistServer::DataHandle, this, _1, _2, _3));
  }

  ~KVStoreDistServer() {
    if (ps_server_) {
      delete ps_server_;
    }
  }

private:

  void DataHandle(const KVMeta& req_meta,
                  const KVPairs<float>& req_data,
                  KVServer<float>* server) {
    CHECK_EQ(1, req_data.keys.size());
    Key key = req_data.keys[0];
    auto& weights = weights_[key];

    size_t n = 0;
    
    if (req_meta.push) { // push
      n = req_data.vals.size();
      if (weights.empty()) { // init
        weights.resize(n);
        for (size_t i = 0; i < n; ++i) {
          weights[i] = req_data.vals[i];
        }
        server->Response(req_meta);
      } else if (sync_){ // sync
        CHECK_EQ(weights.size(), req_data.vals.size());
        auto& merged = merge_buf_[key];
        if (merged.vals.empty()) {
          merged.vals.resize(n, 0);
        }
        CHECK_EQ(merged.vals.size(), req_data.vals.size());
        for (size_t i = 0; i < n; ++i) {
          merged.vals[i] += req_data.vals[i];
        }

        merged.request.push_back(req_meta);
        if (merged.request.size() == (size_t)NumWorkers()) {
          for (size_t i = 0; i < n; ++i) {
            weights[i] += merged.vals[i];
          }
          for (const auto& req : merged.request) {
            server->Response(req);
          }
          merged.request.clear();
          merged.vals.clear();
        }
      } else { // async
        for (size_t i = 0; i < n; ++i) {
          weights[i] += req_data.vals[i];
        }
        server->Response(req_meta);
      }
    } else { // pull
      CHECK(!weights_.empty()) << "Init " << key << " first";      
      n = weights.size();

      KVPairs<float> response;
      response.keys = req_data.keys;
      
      response.vals.resize(n);
      for (size_t i = 0; i < n; ++i) {
        response.vals[i] = weights[i];
      }
      server->Response(req_meta, response);
    }
  }

  struct MergeBuf {
    std::vector<KVMeta> request;
    std::vector<float> vals;
  };

  std::unordered_map<Key, std::vector<float>> weights_;
  std::unordered_map<Key, MergeBuf> merge_buf_;
  bool sync_;
  KVServer<float>* ps_server_;
};

void StartServer() {
  if (!IsServer()) {
    return;
  }
  auto server = new KVStoreDistServer();
  RegisterExitCallback([server](){ delete server; });
}

void RunWorker() {
  if (!IsWorker()) return;
  KVWorker<float> kv(0);

  // init
  std::vector<Key> keys;
  std::vector<float> vals;
  keys.resize(1, 0);
  vals.resize(100, 0);

  if (MyRank() == 0) {
    kv.Wait(kv.Push(keys, vals));
  }

  if (!Postoffice::Get()->is_recovery()) {
    Postoffice::Get()->Barrier(kWorkerGroup);
  }

  for (size_t i=0; i<100; ++i){
    vals[i] = 1;
  }
  kv.Wait(kv.Push(keys, vals));

  // pull
  std::vector<float> rets;
  kv.Wait(kv.Pull(keys, &rets));
  //LL << rets.size() << "qqq";
  //CHECK_EQ(100, rets.size());

  float res = 0;
  for (size_t i = 0; i < 100; ++i) {
    res += fabs(rets[i] - 2*vals[i]);
  }
  //CHECK_LT(res, 1e-5);
  LL << "error: " << res;
}

int main() {
  StartServer();

  Start();
  RunWorker();

  Finalize();
  return 0;
}
