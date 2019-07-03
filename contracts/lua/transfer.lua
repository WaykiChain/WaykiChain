mylib = require "mylib"

ADDR_TYPE = {
  ENUM_REGID = 1, --REG_ID
  ENUM_BASE58 = 2 --BASE58 ADDR
}

OPER_TYPE = {
  ENUM_ADD_FREE = 1,
  ENUM_MINUS_FREE = 2
}

function Unpack(t, i)
  i = i or 1
  if t[i] then
    return t[i], Unpack(t, i + 1)
  end
end

function GetValueFromContract(tbl, start, length)
  local newTab = {}
  local i
  for i = 0, length - 1 do
    newTab[1 + i] = tbl[start + i]
  end
  return newTab
end

function Main()
  local writeOutputTbl = {
    addrType = 1, --account type
    accountIdTbl = {}, --account id
    operatorType = 0, --operate type
    outHeight = 0, --outtime height
    moneyTbl = {} --amount
  }

  local accountTbl = {}
  accountTbl = GetValueFromContract(contract, 1, 34)
  writeOutputTbl.addrType = ADDR_TYPE.ENUM_BASE58
  writeOutputTbl.operatorType = OPER_TYPE.ENUM_ADD_FREE
  writeOutputTbl.accountIdTbl = {Unpack(accountTbl)}
  writeOutputTbl.moneyTbl = {mylib.GetCurTxPayAmount()}
  mylib.WriteOutput(writeOutputTbl)

  writeOutputTbl.addrType = ADDR_TYPE.ENUM_REGID
  writeOutputTbl.operatorType = OPER_TYPE.ENUM_MINUS_FREE
  writeOutputTbl.accountIdTbl = {mylib.GetScriptID()}
  mylib.WriteOutput(writeOutputTbl)
end

Main()
