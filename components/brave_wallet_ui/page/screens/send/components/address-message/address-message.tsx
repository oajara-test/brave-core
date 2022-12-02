// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { AddressMessageInfo } from '../../../../../constants/types'

// Styled Components
import { LearnMoreButton } from './address-message.style'
import { Column, Row, Text, VerticalDivider, VerticalSpacer } from '../../shared.styles'

interface Props {
  addressMessageInfo: AddressMessageInfo
}

export const AddressMessage = (props: Props) => {
  const { addressMessageInfo } = props

  return (
    <Column
      columnWidth='full'
      horizontalPadding={16}
      horizontalAlign='flex-start'
    >
      <VerticalDivider marginBottom={16} />
      <Text
        textSize='14px'
        textColor='text01'
        isBold={true}
        textAlign='left'
      >
        {addressMessageInfo.title}
      </Text>
      {addressMessageInfo.description &&
        <>
          <VerticalSpacer size={8} />
          <Row
            rowWidth='full'
          >
            <Text
              textSize='12px'
              textColor='text03'
              isBold={false}
              textAlign='left'
            >
              {addressMessageInfo.description} {addressMessageInfo.url &&
                <LearnMoreButton
                  href={addressMessageInfo.url}
                  target='_blank'
                  rel='noopener noreferrer'>
                  Learn more
                </LearnMoreButton>
              }
            </Text>
          </Row>
        </>
      }
      <VerticalSpacer size={16} />
    </Column>
  )
}

export default AddressMessage
