// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from 'brave-ui'
import { WalletSelectors } from '../../../common/selectors'
import {
  ParsedTransactionWithoutFiatValues,
  parseTransactionWithoutPrices
} from '../../../common/hooks/transaction-parser'

// hooks
import { useUnsafeWalletSelector } from '../../../common/hooks/use-safe-selector'

// components
import { AccountFilterSelector, NetworkFilterSelector, PortfolioTransactionItem } from '../../../components/desktop'
import { SearchBar } from '../../../components/shared'

// styles
import { Row } from '../../../components/shared/style'
import { useParams } from 'react-router'
import { getNetworkFromTXDataUnion } from '../../../utils/network-utils'

interface Params {
  address?: string
  chainId?: string
  transactionId?: string
}

// export const getParsedTransactions = async ({
//   account,
//   accounts,
//   fullTokenList,
//   transactionNetwork,
//   txService,
//   userVisibleTokensList,
//   solFeeEstimates
// }: {
//   account: WalletAccountType
//   accounts: WalletAccountType[]
//   fullTokenList: BraveWallet.BlockchainToken[]
//   solFeeEstimates?: SolFeeEstimates | undefined
//   transactionNetwork: BraveWallet.NetworkInfo
//   userVisibleTokensList: BraveWallet.BlockchainToken[]
//   txService: WalletApiProxy['txService']
// }
// ) => {
//   const { transactionInfos } = await txService.getAllTransactionInfo(
//     account.coin,
//     account.address
//   )
//   transactionInfos.map(rawTx => {
//     return parseTxWithoutPrices({
//       account,
//       accounts,
//       fullTokenList,
//       transactionNetwork,
//       tx: rawTx,
//       userVisibleTokensList,
//       solFeeEstimates
//     })
//   })
// }

export const TransactionsScreen: React.FC = () => {
  // routing
  const {
    address,
    chainId
    // transactionId
  } = useParams<Params>()

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')

  // redux
  const allTransactions = useUnsafeWalletSelector(WalletSelectors.transactions)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const defaultAccounts = useUnsafeWalletSelector(WalletSelectors.defaultAccounts)
  const networkList = useUnsafeWalletSelector(WalletSelectors.networkList)
  const fullTokenList = useUnsafeWalletSelector(WalletSelectors.fullTokenList)
  const userVisibleTokens = useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)
  const solFeeEstimates = useUnsafeWalletSelector(WalletSelectors.solFeeEstimates)
  const selectedNetwork = useUnsafeWalletSelector(WalletSelectors.selectedNetwork)

  // computed / memos
  const foundAccountFromParam = address
    ? accounts.find(a => a.address === address)
    : undefined

  const txsAllChains = React.useMemo(() => {
    return foundAccountFromParam?.address
      ? allTransactions?.[foundAccountFromParam.address] || []
      : accounts.flatMap(a => allTransactions[a.address] || [])
  }, [foundAccountFromParam?.address, allTransactions, accounts])

  const txsForFilteredChain = React.useMemo(() => {
    return chainId
      ? txsAllChains.filter(tx =>
          getNetworkFromTXDataUnion(tx.txDataUnion, networkList, undefined)?.chainId === chainId
        )
      : txsAllChains
  }, [txsAllChains, chainId, networkList])

  const parsedTransactions = React.useMemo(() => {
    return txsForFilteredChain.map(tx => parseTransactionWithoutPrices({
      accounts,
      fullTokenList,
      transactionNetwork: getNetworkFromTXDataUnion(tx.txDataUnion, networkList, selectedNetwork),
      tx,
      userVisibleTokensList: userVisibleTokens,
      solFeeEstimates
    }))
  }, [
    accounts,
    defaultAccounts,
    foundAccountFromParam,
    fullTokenList,
    networkList,
    selectedNetwork,
    solFeeEstimates,
    txsForFilteredChain,
    userVisibleTokens
  ])

  const filteredTransactions = React.useMemo(() => {
    return filterTransactionsBySearchValue(searchValue, parsedTransactions)
  }, [searchValue, parsedTransactions])

  // render
  return (
    <>
      Transactions
      <Row>
        <SearchBar
          placeholder={getLocale('braveWalletSearchText')}
          action={(e) => setSearchValue(e.target.value)}
          value={searchValue}
        />
        <AccountFilterSelector />
        <NetworkFilterSelector />
      </Row>
      {filteredTransactions.map(tx =>
        <PortfolioTransactionItem
          key={tx.id}
          account={undefined}
          accounts={[]}
          displayAccountName
          transaction={tx}
        />
      )}
    </>
  )
}

export default TransactionsScreen

function filterTransactionsBySearchValue (
  searchValue: string,
  txsForFilteredChain: ParsedTransactionWithoutFiatValues[]
): ParsedTransactionWithoutFiatValues[] {
  return searchValue === ''
    ? txsForFilteredChain
    : txsForFilteredChain.filter((tx) => (
      tx.sender.toLowerCase().includes(searchValue.toLowerCase()) ||
      tx.recipient.toLowerCase().includes(searchValue.toLowerCase()) ||
      tx.senderLabel.toLowerCase().includes(searchValue.toLowerCase()) ||
      tx.recipientLabel.toLowerCase().includes(searchValue.toLowerCase())
      // tx.originInfo?.eTldPlusOne.toLowerCase().includes(searchValue.toLowerCase()) ||
      // tx.originInfo?.originSpec.toLowerCase().includes(searchValue.toLowerCase()) ||
      // tx.txHash.toLowerCase().includes(searchValue.toLowerCase())
    ))
}
