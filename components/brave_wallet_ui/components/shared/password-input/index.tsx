import * as React from 'react'

import {
  StyledWrapper,
  InputWrapper,
  ToggleVisibilityButton,
  Input,
  ErrorText,
  ErrorRow,
  WarningIcon
} from './style'

export interface Props {
  onChange: (value: string) => void
  onKeyDown?: (event: React.KeyboardEvent<HTMLInputElement>) => void
  autoFocus?: boolean
  value?: string
  placeholder: string
  hasError: boolean
  error: string
  showToggleButton?: boolean
}

function PasswordInput (props: Props) {
  const { onChange, onKeyDown, placeholder, error, hasError, autoFocus, value, showToggleButton } = props
  const [showPassword, setShowPassword] = React.useState(false)

  const inputPassword = (event: React.ChangeEvent<HTMLInputElement>) => {
    onChange(event.target.value)
  }

  const onTogglePasswordVisibility = () => {
    setShowPassword(!showPassword)
  }

  return (
    <StyledWrapper>
      <InputWrapper>
        <Input
          hasError={hasError}
          type={(showToggleButton && showPassword) ? 'text' : 'password'}
          placeholder={placeholder}
          value={value}
          onChange={inputPassword}
          onKeyDown={onKeyDown}
          autoFocus={autoFocus}
          />
          {showToggleButton &&
            <ToggleVisibilityButton
              showPassword={showPassword}
              onClick={onTogglePasswordVisibility}
              tabIndex={-1}
            />
          }
      </InputWrapper>
      {hasError &&
        <ErrorRow>
          <WarningIcon />
          <ErrorText>{error}</ErrorText>
        </ErrorRow>
      }
    </StyledWrapper>
  )
}

PasswordInput.defaultProps = {
  showToggleButton: true
}

export default PasswordInput
