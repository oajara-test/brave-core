/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_service.h"

namespace brave_wallet {

using GetRunningTranactionsCountCallback = base::OnceCallback<void(size_t)>;

class TxStatusResolver {
 public:
  explicit TxStatusResolver(TxService* tx_service);
  ~TxStatusResolver();
  void GetRunningTranactionsCount(GetRunningTranactionsCountCallback callback);

 private:
  void RunCheck(GetRunningTranactionsCountCallback callback,
                size_t counter,
                mojom::CoinType coin);
  size_t GetInProgressTransactionsCount(
      const std::vector<mojom::TransactionInfoPtr>& result);

  void OnTxResolved(GetRunningTranactionsCountCallback callback,
                    size_t counter,
                    mojom::CoinType coin,
                    std::vector<mojom::TransactionInfoPtr> result);

  raw_ptr<TxService> tx_service_;
  base::WeakPtrFactory<TxStatusResolver> weak_ptr_factory_{this};
};

}  // namespace brave_wallet
