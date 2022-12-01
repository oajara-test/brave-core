/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import {
  BraveWallet,
  SerializableTimeDelta,
  SerializableTransactionInfo,
  SolFeeEstimates,
  WalletAccountType,
  WalletState
} from '../../constants/types'

// Utils
import { getLocale } from '../../../common/locale'
import Amount from '../../utils/amount'
import {
  getTypedSolanaTxInstructions,
  TypedSolanaInstructionWithParams
} from '../../utils/solana-instruction-utils'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  findTransactionAccount,
  findTransactionToken,
  getETHSwapTranasactionBuyAndSellTokens,
  getFormattedTransactionTransferredValue,
  getGasFeeFiatValue,
  getIsTxApprovalUnlimited,
  getTransactionApprovalTargetAddress,
  getTransactionDecimals,
  getTransactionErc721TokenId,
  getTransactionFiatValues,
  getTransactionIntent,
  getTransactionNonce,
  getTransactionToAddress,
  getTransactionTokenSymbol,
  isFilecoinTransaction,
  isSendingToKnownTokenContractAddress,
  isSolanaSplTransaction,
  isSolanaTransaction,
  isSwapTransaction,
  parseTransactionFeesWithoutPrices,
  transactionHasSameAddressError,
  TransactionInfo
} from '../../utils/tx-utils'
import { getBalance } from '../../utils/balance-utils'
import { getAddressLabel } from '../../utils/account-utils'
import { makeNetworkAsset } from '../../options/asset-options'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'

// Hooks
import usePricing from './pricing'
import { makeSerializableTimeDelta } from '../../utils/model-serialization-utils'
import { SwapExchangeProxy } from '../constants/registry'

interface ParsedTransactionFees {
  gasLimit: string
  gasPrice: string
  maxPriorityFeePerGas: string
  maxFeePerGas: string
  gasFee: string
  gasFeeFiat: string
  isEIP1559Transaction: boolean
  missingGasLimitError?: string
  gasPremium?: string
  gasFeeCap?: string
}

export interface ParsedTransaction extends ParsedTransactionFees, Pick<BraveWallet.TransactionInfo, 'txType'> {
  // Common fields
  id: string
  hash: string
  nonce: string
  createdTime: SerializableTimeDelta
  status: BraveWallet.TransactionStatus
  sender: string
  senderLabel: string
  recipient: string
  recipientLabel: string
  fiatValue: Amount
  fiatTotal: Amount
  formattedNativeCurrencyTotal: string
  value: string
  valueExact: string
  weiTransferredValue: string
  symbol: string
  decimals: number // network decimals
  insufficientFundsForGasError?: boolean
  insufficientFundsError?: boolean
  contractAddressError?: string
  sameAddressError?: string
  erc721TokenId?: string
  isSwap?: boolean
  intent: string
  chainId: string

  // Tx type flags
  isSolanaTransaction: boolean
  isSolanaDappTransaction: boolean
  isSolanaSPLTransaction: boolean
  isFilecoinTransaction: boolean
  coinType: BraveWallet.CoinType

  // Tokens
  token?: BraveWallet.BlockchainToken
  erc721BlockchainToken?: BraveWallet.BlockchainToken

  // Token approvals
  approvalTarget?: string
  approvalTargetLabel?: string
  isApprovalUnlimited?: boolean

  // Swap
  sellToken?: BraveWallet.BlockchainToken
  sellAmount?: Amount
  sellAmountWei?: Amount
  buyToken?: BraveWallet.BlockchainToken
  minBuyAmount?: Amount
  minBuyAmountWei?: Amount
  // sending to 0x Exchange Proxy
  isSendingToZeroXExchangeProxy: boolean

  // Solana Dapp Instructions
  instructions?: TypedSolanaInstructionWithParams[]
}

export function useTransactionFeesParser (selectedNetwork?: BraveWallet.NetworkInfo, networkSpotPrice?: string, solFeeEstimates?: SolFeeEstimates) {
  return React.useCallback((transactionInfo: TransactionInfo): ParsedTransactionFees => {
    const txFeesBase = parseTransactionFeesWithoutPrices(
      transactionInfo,
      solFeeEstimates
    )

    return {
      ...txFeesBase,
      gasFeeFiat: getGasFeeFiatValue({
        gasFee: txFeesBase.gasFee,
        networkSpotPrice,
        txNetwork: selectedNetwork
      }),
      missingGasLimitError: txFeesBase.isMissingGasLimit
        ? getLocale('braveWalletMissingGasLimitError')
        : undefined
    }
  }, [selectedNetwork, networkSpotPrice])
}

export function useTransactionParser (
  transactionNetwork?: BraveWallet.NetworkInfo
) {
  const {
    selectedNetwork: reduxSelectedNetwork,
    fullTokenList,
    userVisibleTokensInfo: visibleTokens,
    accounts,
    transactionSpotPrices: spotPrices,
    solFeeEstimates
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
  const selectedNetwork = transactionNetwork || reduxSelectedNetwork
  const { findAssetPrice } = usePricing(spotPrices)

  const networkSpotPrice = React.useMemo(
    () => selectedNetwork
      ? findAssetPrice(selectedNetwork.symbol)
      : '',
    [selectedNetwork, findAssetPrice]
  )

  return React.useCallback((tx: SerializableTransactionInfo): ParsedTransaction => {
    const {
      token,
      gasFee,
      sellToken,
      weiTransferredValue,
      value: normalizedTransferredValue,
      ...txBase
    } = parseTransactionWithoutPrices({
      accounts,
      fullTokenList,
      transactionNetwork: selectedNetwork,
      tx,
      userVisibleTokensList: visibleTokens,
      solFeeEstimates
    })

    return {
      token,
      gasFee,
      sellToken,
      weiTransferredValue,
      value: normalizedTransferredValue,
      ...txBase,
      ...getTransactionFiatValues({
        gasFee,
        networkSpotPrice,
        normalizedTransferredValue,
        spotPrices,
        tx,
        sellToken,
        token,
        txNetwork: selectedNetwork,
        transferredValueWei: weiTransferredValue
      })
    }
  }, [
    selectedNetwork,
    accounts,
    spotPrices
  ])
}

export type ParsedTransactionWithoutFiatValues = Omit<
  ParsedTransaction,
  | 'fiatValue'
  | 'fiatTotal'
  | 'formattedNativeCurrencyTotal'
  | 'normalizedTransferredValue'
  | 'gasFeeFiat'
>

export function parseTransactionWithoutPrices ({
  accounts,
  fullTokenList,
  tx,
  transactionNetwork,
  userVisibleTokensList,
  solFeeEstimates
}: {
  accounts: WalletAccountType[]
  fullTokenList: BraveWallet.BlockchainToken[]
  solFeeEstimates?: SolFeeEstimates
  tx: TransactionInfo
  transactionNetwork?: BraveWallet.NetworkInfo
  userVisibleTokensList: BraveWallet.BlockchainToken[]
}): ParsedTransactionWithoutFiatValues {
  const to = getTransactionToAddress(tx)
  const combinedTokensList = userVisibleTokensList.concat(fullTokenList)
  const token = findTransactionToken(tx, combinedTokensList)
  const nativeAsset = transactionNetwork ? makeNetworkAsset(transactionNetwork) : undefined
  const account = findTransactionAccount(accounts, tx)
  const accountNativeBalance = getBalance(account, nativeAsset)
  const accountTokenBalance = getBalance(account, token)

  const {
    buyToken,
    sellToken,
    buyAmount,
    sellAmount,
    sellAmountWei,
    buyAmountWei
  } = getETHSwapTranasactionBuyAndSellTokens({
    nativeAsset,
    tokensList: combinedTokensList,
    tx
  })

  const {
    normalizedTransferredValue,
    normalizedTransferredValueExact,
    weiTransferredValue
  } = getFormattedTransactionTransferredValue({
    tx,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const erc721BlockchainToken = [
    BraveWallet.TransactionType.ERC721TransferFrom,
    BraveWallet.TransactionType.ERC721SafeTransferFrom
  ].includes(tx.txType) ? token : undefined

  const approvalTarget = getTransactionApprovalTargetAddress(tx)

  const {
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    isMissingGasLimit,
    maxFeePerGas,
    maxPriorityFeePerGas,
    isEIP1559Transaction
  } = parseTransactionFeesWithoutPrices(tx, solFeeEstimates)

  const insufficientFundsError = accountHasInsufficientFundsForTransaction({
    accountNativeBalance,
    accountTokenBalance,
    gasFee,
    tokensList: combinedTokensList,
    tx,
    account,
    nativeAsset
  })

  const erc721TokenId = getTransactionErc721TokenId(tx)

  const missingGasLimitError = isMissingGasLimit
    ? getLocale('braveWalletMissingGasLimitError')
    : undefined

  const approvalTargetLabel = getAddressLabel(approvalTarget, accounts)
  const coinType = getCoinFromTxDataUnion(tx.txDataUnion)
  const createdTime = makeSerializableTimeDelta(tx.createdTime)

  const contractAddressError = isSendingToKnownTokenContractAddress(tx, combinedTokensList)
    ? getLocale('braveWalletContractAddressError')
    : undefined

  const decimals = getTransactionDecimals({
    tx,
    network: transactionNetwork,
    sellToken,
    erc721Token: erc721BlockchainToken,
    token
  })

  const instructions = getTypedSolanaTxInstructions(tx.txDataUnion.solanaTxData)

  const sameAddressError = transactionHasSameAddressError(tx)
    ? getLocale('braveWalletSameAddressError')
    : undefined

  const symbol = getTransactionTokenSymbol({
    tx,
    txNetwork: transactionNetwork,
    token,
    sellToken
  })

  const intent = getTransactionIntent({
    normalizedTransferredValue,
    tx,
    buyAmount,
    buyToken,
    erc721TokenId,
    sellAmount,
    sellToken,
    token,
    transactionNetwork
  })

  const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
    accountNativeBalance,
    gasFee
  })

  const isSendingToZeroXExchangeProxy = tx.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() ===
    SwapExchangeProxy
  return {
    approvalTarget,
    approvalTargetLabel,
    buyToken,
    chainId: transactionNetwork?.chainId || '',
    coinType,
    contractAddressError,
    createdTime,
    decimals,
    erc721BlockchainToken,
    erc721TokenId,
    gasFee,
    gasFeeCap,
    gasLimit,
    gasPremium,
    gasPrice,
    hash: tx.txHash,
    id: tx.id,
    instructions,
    insufficientFundsError,
    insufficientFundsForGasError,
    intent,
    isApprovalUnlimited: getIsTxApprovalUnlimited(tx),
    isEIP1559Transaction,
    isFilecoinTransaction: isFilecoinTransaction(tx),
    isSendingToZeroXExchangeProxy,
    isSolanaDappTransaction: isSolanaTransaction(tx),
    isSolanaSPLTransaction: isSolanaSplTransaction(tx),
    isSolanaTransaction: isSolanaTransaction(tx),
    isSwap: isSwapTransaction(tx),
    maxFeePerGas,
    maxPriorityFeePerGas,
    minBuyAmount: buyAmount,
    minBuyAmountWei: buyAmountWei,
    missingGasLimitError,
    nonce: getTransactionNonce(tx),
    recipient: to,
    recipientLabel: getAddressLabel(to, accounts),
    sameAddressError,
    sellAmount,
    sellAmountWei,
    sellToken,
    sender: tx.fromAddress,
    senderLabel: getAddressLabel(tx.fromAddress, accounts),
    status: tx.txStatus,
    symbol,
    token,
    txType: tx.txType,
    value: normalizedTransferredValue,
    valueExact: normalizedTransferredValueExact,
    weiTransferredValue
  }
}
