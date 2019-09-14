mylib = require "mylib"

--模块功能:足彩，篮彩投注 v2

--日志类型
LOG_TYPE =
{
   ENUM_STRING = 0, --字符串类型
   ENUM_NUMBER = 1, --数字类型
};

--系统账户操作定义
OPER_TYPE =
{
	ENUM_ADD_FREE 	= 1,   	--系统账户加
	ENUM_MINUS_FREE = 2  	--系统账户减
}

--脚本应用账户操作类型定义
APP_OPERATOR_TYPE =
{
	ENUM_ADD_FREE_OP = 1,      --自由账户加
	ENUM_SUB_FREE_OP = 2,      --自由账户减
	ENUM_ADD_FREEZED_OP = 3,   --冻结账户加
	ENUM_SUB_FREEZED_OP = 4    --冻结账户减
}

--账户类型
ADDR_TYPE =
{
	ENUM_REGID  = 1,		-- REG_ID
	ENUM_BASE58 = 2,		-- BASE58 ADDR
}

--交易类型
TX_TYPE =
{
	TX_GAME_CFG = 1,	--赛事配置
	TX_BET = 2, 		--投注
	TX_END_BET = 3,		--结束投注
	TX_RUN_LOTTERY = 4,	--开奖
	TX_SEND_PRIZE = 5,	--派奖
	TX_PUBLISH = 6,		--发行资产
	TX_SEND_ASSET = 7,	--转发资产
	TX_GAME_TERM = 8	--终止赛事
}

--彩种
LOTTERY_TYPE =
{
	ENUM_FOOTBALL = 1,		--足彩
	ENUM_BASKETBALL = 2		--篮彩
}

--足彩玩法
FOOTBALL_PLAY_TYPE =
{
	ENUM_SPF = 1,			--胜平负
	ENUM_TOTAL_NUM = 2,		--总进球
	ENUM_ODD_EVEN = 3,		--单双
	ENUM_HALF = 4			--半全场
}

--胜平负投注方案
FOOTBALL_SPF_TYPE =
{
	ENUM_WIN = 1,			--主胜
	ENUM_EVEN = 2,			--平
	ENUM_LOSE = 3			--主负
}

--总进球投注方案
FOOTBALL_TOTAL_TYPE =
{
	ENUM_0 = 1,				--大于2.5个
	ENUM_1 = 2				--小于2.5个
}

--单双投注方案
FOOTBALL_ODD_EVEN_TYPE =
{
	ENUM_ODD = 1,			--单
	ENUM_EVEN = 2			--双
}


--半全场投注方案
FOOTBALL_HALF_TYPE =
{
	ENUM_GOAL	 	= 1,	--进球
	ENUM_NO_GOAL 	= 2		--没有进球
}

--篮球玩法
BASKETBALL_PLAY_TYPE =
{
	ENUM_SF = 1,			--胜负
	ENUM_RSF = 2,			--让分胜负
	ENUM_TOTAL_SCORE = 3,	--总得分
	ENUM_ODD_EVEN = 4		--单双
}

--胜负投注方案
BASKETBALL_SF_TYPE =
{
	ENUM_WIN = 1,	--胜
	ENUM_LOSE = 2	--负
}

--让分胜负投注方案
BASKETBALL_RSF_TYPE =
{
	ENUM_WIN = 1,	--胜
	ENUM_LOSE = 2	--负
}

--总得分投注方案
BASKETBALL_TOTAL_SCORE_TYPE =
{
	ENUM_GREATER	 = 1,		--大于215.5分
	ENUM_LESS		 = 2		--小于215.5分
}

--单双
FOOTBALL_ODD_EVEN_TYPE =
{
	ENUM_ODD = 1,			--单
	ENUM_EVEN = 2			--双
}


--判断表是否为空
function TableIsEmpty(t)
	return _G.next(t) == nil
end

--判断表非空
function TableIsNotEmpty(t)
    return false == TableIsEmpty(t)
end

--[[
  功能:遍历表元素
  参数：
	t:表
	i:开始索引
--]]
function Unpack(t,i)
   i = i or 1
   if t[i] then
     return t[i],Unpack(t,i+1)
   end
end

--[[
  功能:日志输出
  参数：
	aKey:日志类型
	bLength:日志长度
	cValue：日志
--]]
function LogPrint(aKey,bLength,cValue)
	assert(bLength >= 1,"LogPrint bLength invlaid")
	if(aKey == LOG_TYPE.ENUM_STRING) then
      assert(type(cValue) == "string","LogPrint cValue invlaid0")
	elseif(aKey == LOG_TYPE.ENUM_NUMBER)	 then
	    assert(TableIsNotEmpty(cValue),"LogPrint cValue invlaid1")
	else
	    error("LogPrint aKey invlaid")
	end

	local logTable = {
		key = LOG_TYPE.ENUM_STRING,
		length = 0,
		value = nil
    }
	logTable.key = aKey
	logTable.length = bLength
	logTable.value = cValue
	mylib.LogPrint(logTable)
end

--[[
  功能：检查脚本数据库中以key的value是否存在
  参数：
	无
  返回值：
	如果value存在，则返回true和value
	如果value不存在，则返回false和nil
--]]
function CheckIsConfigKey(key)

	assert(#key > 1,"Key is empty")
	local valueTbl = {mylib.ReadData(key)}

	if TableIsNotEmpty(valueTbl) then
		return true,valueTbl
	else
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Check key empty"),"Check key empty")
		return false,nil
	end

	return false,nil
end

--[[
  功能:获取表从开始索引指定长度的元素集合
  参数：
	tbl:表
	start:开始索引
	length:长度
  返回值：
	一个新表
--]]
function GetValueFromArray(tbl,start,length)
  assert(start > 0,"GetValueFromArray start err")
  assert(length > 0,"GetValueFromArray length err")
  local newTab = {}
	local i
	for i = 0,length -1 do
		newTab[1 + i] = tbl[start + i]
	end
    return newTab
end

--[[
  功能:内存拷贝
--]]
function MemCpy(tDest,start0,tSrc,start1,length)
  assert(tDest ~= nil,"tDest is empty")
  assert(TableIsNotEmpty(tSrc),"tDest is empty")
  assert(start0 > 0,"start0 err")
  assert(start1 > 0,"start1 err")
  assert(length > 0,"length err")
  local i
  for i = 1,length do
    tDest[start0 + i -1] = tSrc[start1 + i - 1]
  end
end


--[[
  功能:比较两表元素是否相同
  参数：
	tDest:表1
	tSrc:表2
	start1:开始索引
  返回值：
	0:相等
	1:表1>表2
	2:表1<表2
--]]
function MemCmp(tDest,tSrc,start1)
	assert(TableIsNotEmpty(tDest),"tDest is empty")
	assert(TableIsNotEmpty(tSrc),"tSrc is empty")
	local i
	for i = #tDest,1,-1 do
		if tDest[i] > tSrc[i + start1 - 1] then
			return 1
		elseif 	tDest[i] < tSrc[i + start1 - 1] then
			return 2
		else

		end
	end
	return 0
end

--字符转换拼接
function serialize(obj, hex)
    local lua = ""
    local t = type(obj)

    if t == "table" then
		for i=1, #obj do
			if hex == false then
				lua = lua .. string.format("%c",obj[i])
			else
				lua = lua .. string.format("%02x",obj[i])
			end
		end
    elseif t == "nil" then
        return nil
    else
        error("can not serialize a " .. t .. " type.")
    end

    return lua
end

--资产发行
--输入数据结构：转移地址，资产数，众筹量，冻结周期，冻结期数
function Publish()
  local idTbl =
  {
	idLen = 0,
	idValueTbl = {}
  }

  local valueTbl = {}
  local isConfig = false
  isConfig,valueTbl = CheckIsConfigKey("config")
  if isConfig then
	LogPrint(LOG_TYPE.ENUM_STRING,string.len("Already configured"),"Already configured")
	error("Already configured")
  end

  local toAddrTbl = {}
  toAddrTbl = GetValueFromArray(contract, 3, 34)

  local moneyTbl = {}
  moneyTbl = GetValueFromArray(contract, 37, 8)
  local money = mylib.ByteToInteger(Unpack(moneyTbl))
  assert(money > 0,"money <= 0")


  local crowdMoneyTbl = {}
  crowdMoneyTbl = GetValueFromArray(contract, 45, 8)
  local crowdMoney = mylib.ByteToInteger(Unpack(crowdMoneyTbl))
  assert(crowdMoney > 0,"crowdMoney <= 0")

  assert(money >= crowdMoney,"There's not enough money in the account.")

  local freezeMoney = money - crowdMoney

  local peroidTbl = {}
  peroidTbl = GetValueFromArray(contract, 53, 4)
  local period = mylib.ByteToInteger(Unpack(peroidTbl))
  if period > 0 then
	assert(period > 0,"period <= 0")
  end

  local nNumTbl = {}
  nNumTbl = GetValueFromArray(contract, 57, 4)
  local nNum = mylib.ByteToInteger(Unpack(nNumTbl))
  if nNum > 0 then
    assert(nNum > 0,"nNum <= 0")
  end

  --获取每期的冻结量
  local eachFreezeMoneyTbl = {}
  local nEachFreezeMoney,_ =  math.modf(freezeMoney / nNum)
  eachFreezeMoneyTbl = {mylib.IntegerToByte8(nEachFreezeMoney)}

  local appOperateTbl = {
		operatorType = 0,
		outHeight = 0,
		moneyTbl = {},
		userIdLen = 0,
		userIdTbl = {},
		fundTagLen = 0,   --fund tag len
		fundTagTbl = {}   --fund tag
	}

  idTbl.idLen = 34
  idTbl.idValueTbl = toAddrTbl

  appOperateTbl.operatorType = APP_OPERATOR_TYPE.ENUM_ADD_FREE_OP
  appOperateTbl.userIdLen = idTbl.idLen
  appOperateTbl.userIdTbl = idTbl.idValueTbl
  appOperateTbl.moneyTbl = crowdMoneyTbl

  assert(mylib.WriteOutAppOperate(appOperateTbl),"WriteOutAppOperate err1")

  if nEachFreezeMoney > 0 then
    local curHeight = 0
    curHeight = mylib.GetCurRunEnvHeight()

    appOperateTbl.operatorType = APP_OPERATOR_TYPE.ENUM_ADD_FREEZED_OP
    appOperateTbl.moneyTbl = eachFreezeMoneyTbl

    for i = 1, nNum do
		appOperateTbl.outHeight = curHeight + period * (i - 1)
		assert(mylib.WriteOutAppOperate(appOperateTbl),"WriteOutAppOperate err2")
    end
  end

  local writeDbTbl = {
    key = "config",
    length = 8 + 8 + 34,
    value = {}
  }

  writeDbTbl.value = GetValueFromArray(contract,3,50)

  if not mylib.WriteData(writeDbTbl) then
    LogPrint(LOG_TYPE.ENUM_STRING,string.len("SaveConfig WriteData error"),"SaveConfig WriteData error")
    error("SaveConfig WriteData error")
  end

end

--转发资产
--输入数据结构：转移地址，资产数
function Send()
  -- 1）
  local valueTbl = {}
  local isConfig = false
  isConfig,valueTbl = CheckIsConfigKey("config")
  if not isConfig then
	LogPrint(LOG_TYPE.ENUM_STRING,string.len("Not configured"),"Not configured")
	error("Not configured")
  end

  local accountTbl = {0, 0, 0, 0, 0, 0}
  accountTbl = {mylib.GetCurTxAccount()}
  assert(TableIsNotEmpty(accountTbl),"GetCurTxAccount error")

  local base58Addr = {}
  base58Addr = {mylib.GetBase58Addr(Unpack(accountTbl))}
  assert(TableIsNotEmpty(base58Addr),"GetBase58Addr error")

  -- 获取要转币地址
  local toAddrTbl = {}
  toAddrTbl = GetValueFromArray(contract, 3, 34)

  -- 获取转币量
  local moneyTbl = {}
  moneyTbl = GetValueFromArray(contract, 37, 8)

  SendtoAsset(base58Addr, toAddrTbl, moneyTbl)
end

--由A向B转发资产
--输入数据结构：发送地址，转移地址，资产数
function SendtoAsset(fromAddrTbl, toAddrTbl, moneyTbl)
  local money = mylib.ByteToInteger(Unpack(moneyTbl))
  assert(money > 0,"money <= 0")

  local idTbl =
  {
	idLen = 0,
	idValueTbl = {}
  }
  -- 检查账户中能够转移的数量
  idTbl.idLen = #fromAddrTbl
  idTbl.idValueTbl = fromAddrTbl
  local freeMoneyTbl = {mylib.GetUserAppAccValue(idTbl)}
  assert(TableIsNotEmpty(freeMoneyTbl),"GetUserAppAccValue error")
  local freeMoney = mylib.ByteToInteger(Unpack(freeMoneyTbl))

  assert(freeMoney >= money,"There's not enough money in the account.")

  local appOperateTbl = {
		operatorType = 0,  -- 操作类型
		outHeight = 0,    -- 超时高度
		moneyTbl = {},
		userIdLen = 0,    -- 地址长度
		userIdTbl = {},   -- 地址
		fundTagLen = 0,   --fund tag len
		fundTagTbl = {}   --fund tag
	}

  appOperateTbl.operatorType = APP_OPERATOR_TYPE.ENUM_SUB_FREE_OP
  appOperateTbl.userIdLen = idTbl.idLen
  appOperateTbl.userIdTbl = idTbl.idValueTbl
  appOperateTbl.moneyTbl = moneyTbl
  assert(mylib.WriteOutAppOperate(appOperateTbl),"WriteOutAppOperate err1")

  idTbl.idLen = #toAddrTbl
  idTbl.idValueTbl = toAddrTbl
  appOperateTbl.userIdLen = idTbl.idLen
  appOperateTbl.userIdTbl = idTbl.idValueTbl
  appOperateTbl.operatorType = APP_OPERATOR_TYPE.ENUM_ADD_FREE_OP
  assert(mylib.WriteOutAppOperate(appOperateTbl),"WriteOutAppOperate err2")

end

function ZeroMemory(tbl, start, length)
	for i = start,start + length do
		tbl[i] = 0
	end
end

function InitPlayInfo(KeySer, key, playType, start, length)
	local writeDbTbl =
	{
		key="",
		length = 0,
		value = {}
	}

	writeDbTbl.key = key

	MemCpy(writeDbTbl.value, 1, KeySer, 1, 36) --彩票ID
	writeDbTbl.value[37] = playType
	writeDbTbl.value[38] = 0 --开奖结果

	ZeroMemory(writeDbTbl.value, start, length)

	writeDbTbl.value[start + length] = 0
	writeDbTbl.length = start + length

	if not mylib.WriteData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("SaveConfig spf info error"),"SaveConfig spf info error")
		error("SaveConfig spf info error")
	end

end

function InitSession(KeySer, lid, length)
	local writeDbTbl =
	{
		key="",  --彩票ID
		length = 0,
		value = {}
	}

	writeDbTbl.key = lid

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == false,"User already configured")

	local accountTbl = {0, 0, 0, 0, 0, 0}
	accountTbl = {mylib.GetCurTxAccount()}
	assert(TableIsNotEmpty(accountTbl),"GetCurTxAccount error")

	local base58Addr = {}
	base58Addr = {mylib.GetBase58Addr(Unpack(accountTbl))}
	assert(TableIsNotEmpty(base58Addr),"GetBase58Addr error")

	MemCpy(writeDbTbl.value, 1, base58Addr, 1, 34)
	MemCpy(writeDbTbl.value, 35, contract, 39, 35)
	writeDbTbl.value[70] = 1 --可以投注
	writeDbTbl.value[71] = 0 --是否已开奖
	MemCpy(writeDbTbl.value, 72, contract, 74, length)
	writeDbTbl.length = 71 + length

	if not mylib.WriteData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("SaveConfig lottery info error"),"SaveConfig lottery info error")
		error("SaveConfig lottery info error")
	end
end

--赛事配置
--前端下发的数据结构：
--彩票ID[36] + 平台地址[34] + 彩种[1] 1+ 返奖率[4] + 赛事[50]180 + 日期[4] + 主队[20]60 + 客队[20]60 + 场次[10]60
--彩票ID[36] + 平台地址[34] + 彩种[1] 2+ 返奖率[4] + 赛事[50] + 日期[4] + 主队[20] + 客队[20] + 场次[10]

--保存字段：管理员地址[34] + 平台地址[34] + 彩种[1] + 是否可以投注[1] +　保留字段[1] + 返奖率后面的字段
function GameCfg()

	--注册的KEY
	local KeySer = {}
	KeySer = GetValueFromArray(contract, 3, 36)
	local lid = {} --彩票ID
	lid = serialize(KeySer, false)

	local lType = contract[73] --彩种

	if lType == LOTTERY_TYPE.ENUM_FOOTBALL then
		if #contract ~= 441 then
			error("GameCfg contract length err.")
		end
		--初始化足彩赛事信息
		InitSession(KeySer, lid, 368)

		--胜平负玩法信息
		InitPlayInfo(KeySer, lid.."01", 1, 39, 44)

		--总进球玩法信息
		InitPlayInfo(KeySer, lid.."02", 2, 39, 32)

		--单双玩法信息
		InitPlayInfo(KeySer, lid.."03", 3, 39, 32)

		--是否进球玩法信息
		InitPlayInfo(KeySer, lid.."04", 4, 39, 32)
	elseif lType == LOTTERY_TYPE.ENUM_BASKETBALL then
		if #contract ~= 441 then
			error("GameCfg contract length err.")
		end
		--初始化篮彩赛事信息
		InitSession(KeySer, lid, 368)

		--胜负玩法信息
		InitPlayInfo(KeySer, lid.."01", 1, 39, 32)

		--让分胜负玩法信息
		InitPlayInfo(KeySer, lid.."02", 2, 39, 32)

		--总得分玩法信息
		InitPlayInfo(KeySer, lid.."03", 3, 39, 32)

		--单双玩法信息
		InitPlayInfo(KeySer, lid.."04", 4, 39, 32)
	else
		error("lottery type error!")
	end

end


function UpdateBetInfo(betKey, betInfo, timesTbl, moneyTbl, inputTimes, inputMoney)
	local writeDbTbl =
	{
		key="",
		length = 86,
		value = {}
	}

	writeDbTbl.key = betKey

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)

	if isRegister then
		local timesTbl2 = {}
		timesTbl2 = GetValueFromArray(RegisterTb, 74, 4)
		local times = mylib.ByteToInteger(Unpack(timesTbl2))
		assert(times >= 0,"times < 0")

		local moneyTbl2 = {}
		moneyTbl2 = GetValueFromArray(RegisterTb, 78, 8)
		local money = mylib.ByteToInteger(Unpack(moneyTbl2))
		assert(money > 0,"money <= 0")

		times = times + inputTimes
		timesTbl2 = {mylib.IntegerToByte4(times)}

		money = money + inputMoney
		moneyTbl2 = {mylib.IntegerToByte8(money)}

		MemCpy(writeDbTbl.value, 1, RegisterTb, 1, 73)
		MemCpy(writeDbTbl.value, 74, timesTbl2, 1, 4)
		MemCpy(writeDbTbl.value, 78, moneyTbl2, 1, 8)
		writeDbTbl.value[86] = 0

		if not mylib.ModifyData(writeDbTbl) then
			LogPrint(LOG_TYPE.ENUM_STRING,string.len("Modify info error"),"Modify info error")
			error("Modify info error")
		end
	else
		MemCpy(writeDbTbl.value, 1, betInfo, 1, 73)
		MemCpy(writeDbTbl.value, 74, timesTbl, 1, 4)
		MemCpy(writeDbTbl.value, 78, moneyTbl, 1, 8)
		writeDbTbl.value[86] = 0
		if not mylib.WriteData(writeDbTbl) then
			LogPrint(LOG_TYPE.ENUM_STRING,string.len("SaveConfig info error"),"SaveConfig info error")
			error("SaveConfig info error")
		end
	end

end

function UpdatePlayInfo(key, playNum, betType, inputMoney, returnRate)
	local writeDbTbl =
	{
		key="",
		length = 0,
		value = {}
	}

	writeDbTbl.key = key

	if betType < 1 or betType > playNum then
		error("betType >= 1 and betType <= playNum")
	end


	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)

	if isRegister == false then
		error("session non-existent")
	end

	local poolMoneyTbl = {}
	local poolMoney = 0
	poolMoneyTbl = GetValueFromArray(RegisterTb, 39, 8)
	local poolMoney = mylib.ByteToInteger(Unpack(poolMoneyTbl))
	assert(poolMoney >= 0,"poolMoney < 0")


	poolMoney = inputMoney + poolMoney
	poolMoneyTbl = {mylib.IntegerToByte8(poolMoney)}

	--local s = string.format("%d", #RegisterTb)
	--LogPrint(LOG_TYPE.ENUM_STRING,string.len(s),s)

	--MemCpy(writeDbTbl.value, 1, RegisterTb, 1, #RegisterTb)
	writeDbTbl.value = RegisterTb
	MemCpy(writeDbTbl.value, 39, poolMoneyTbl, 1, 8)

	--print(playNum)

	for i = 1,playNum do
		local eachMoneyTbl = {}
		local eachMoney = 0
		eachMoneyTbl = GetValueFromArray(RegisterTb, 47 + 8 * (i - 1), 8)
		eachMoney = mylib.ByteToInteger(Unpack(eachMoneyTbl))
		assert(eachMoney >= 0,"poolMoney < 0")

		if i == betType then
			eachMoney = eachMoney + inputMoney
			eachMoneyTbl = {mylib.IntegerToByte8(eachMoney)}
		end

		local oddsTbl = {}
		local odds = 0

		if eachMoney ~= 0 then
			odds = math.floor(returnRate * poolMoney / eachMoney)
			oddsTbl = {mylib.IntegerToByte4(odds)}
		else
			oddsTbl = {0, 0, 0, 0}
		end

		MemCpy(writeDbTbl.value, 47 + 8 * (i - 1), eachMoneyTbl, 1, 8)
		MemCpy(writeDbTbl.value, 47 + 8 * playNum + 4 * (i - 1), oddsTbl, 1, 4)
	end

	writeDbTbl.length = 47 + playNum * 8 + playNum * 4

	if not mylib.ModifyData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Modify info error"),"Modify info error")
		error("Modify info error")
	end

end


function EachBetting(lid, base58Addr, keyser, addr, lType, playType, betType, returnRate, timesTbl, moneyTbl)
	local betInfo = {}
	MemCpy(betInfo, 1, keyser, 1, 36)
	MemCpy(betInfo, 37, addr, 1, 34)
	local info = serialize(betInfo, false)
	info = info .. string.format("%02x",lType)
	info = info .. string.format("%02x",playType)
	info = info .. string.format("%02x",betType)

	betInfo[71] = lType
	betInfo[72] = playType
	betInfo[73] = betType

	local inputTimes = mylib.ByteToInteger(Unpack(timesTbl))
	assert(inputTimes >= 0,"inputTimes < 0")

	local inputMoney = mylib.ByteToInteger(Unpack(moneyTbl))
	assert(inputMoney > 0,"inputMoney <= 0")


	local idTbl =
	{
		idLen = 0,
		idValueTbl = {}
	}
	-- 检查账户中能够转移的数量
	idTbl.idLen = #base58Addr
	idTbl.idValueTbl = base58Addr
	local freeMoneyTbl = {mylib.GetUserAppAccValue(idTbl)}
	assert(TableIsNotEmpty(freeMoneyTbl),"GetUserAppAccValue error")
	local freeMoney = mylib.ByteToInteger(Unpack(freeMoneyTbl))

	assert(freeMoney >= inputMoney,"There's not enough money in the account.")


	local betKeyTbl = {mylib.Sha256(info)}
	local betKey = serialize(betKeyTbl, true)
	print(betKey)

	--LogPrint(LOG_TYPE.ENUM_STRING,string.len(info),info)
	--LogPrint(LOG_TYPE.ENUM_NUMBER,#betKey,betKey)
	LogPrint(LOG_TYPE.ENUM_STRING,string.len(betKey),betKey)

	--更新投注信息
	UpdateBetInfo(betKey, betInfo, timesTbl, moneyTbl, inputTimes, inputMoney)

	--更新玩法信息
	if lType == LOTTERY_TYPE.ENUM_FOOTBALL then

		if playType == FOOTBALL_PLAY_TYPE.ENUM_SPF then
			--胜平负玩法信息
			UpdatePlayInfo(lid.."01", 3, betType, inputMoney, returnRate)
		elseif playType == FOOTBALL_PLAY_TYPE.ENUM_TOTAL_NUM then
			--总进球玩法信息
			UpdatePlayInfo(lid.."02", 2, betType, inputMoney, returnRate)
		elseif playType == FOOTBALL_PLAY_TYPE.ENUM_ODD_EVEN then
			--单双玩法信息
			UpdatePlayInfo(lid.."03", 2, betType, inputMoney, returnRate)
		elseif playType == FOOTBALL_PLAY_TYPE.ENUM_HALF then
			--半全场玩法信息
			UpdatePlayInfo(lid.."04", 2, betType, inputMoney, returnRate)
		else
			error("football play type error")
		end

	elseif lType == LOTTERY_TYPE.ENUM_BASKETBALL then

		if playType == BASKETBALL_PLAY_TYPE.ENUM_SF then
			--胜负玩法信息
			UpdatePlayInfo(lid.."01", 2, betType, inputMoney, returnRate)
		elseif playType == BASKETBALL_PLAY_TYPE.ENUM_RSF then
			--让分胜负玩法信息
			UpdatePlayInfo(lid.."02", 2, betType, inputMoney, returnRate)
		elseif playType == BASKETBALL_PLAY_TYPE.ENUM_TOTAL_SCORE then
			--总得分玩法信息
			UpdatePlayInfo(lid.."03", 2, betType, inputMoney, returnRate)
		elseif playType == BASKETBALL_PLAY_TYPE.ENUM_ODD_EVEN then
			--单双玩法信息
			UpdatePlayInfo(lid.."04", 2, betType, inputMoney, returnRate)
		else
			error("basketball play type error")
		end

	else
		error("lottery type error!")
	end


	SendtoAsset(base58Addr, keyser, moneyTbl)

end


--投注
--前端下发的数据结构：彩票ID[36] + 用户地址[34] + 彩种[1] + 项数（4字节） +  玩法[1] + 投注方案[1] + 倍数[4] + 金额[8]
function Betting()
	local keyser = {}
	keyser = GetValueFromArray(contract, 3, 36)

	local lid = {}
	lid = serialize(keyser, false)


	local addr = {}
	addr = GetValueFromArray(contract, 39, 34)

	local writeDbTbl =
	{
		key="",
		length = 0,
		value = {}
	}

	--判断赛事是否已经配置
	writeDbTbl.key = lid

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == true,"User not configured")
	assert(RegisterTb[70] == 1, "End betting")
	if RegisterTb[71] == 2 then
		error("The event has been terminated.")
	end

	--获取返奖率
	local returnRateTbl = {}
	returnRateTbl = GetValueFromArray(RegisterTb, 72, 4)
	local returnRate = mylib.ByteToInteger(Unpack(returnRateTbl))
	assert(returnRate > 0,"returnRate <= 0")

	local accountTbl = {0, 0, 0, 0, 0, 0}
	accountTbl = {mylib.GetCurTxAccount()}
	assert(TableIsNotEmpty(accountTbl),"GetCurTxAccount error")

	local base58Addr = {}
	base58Addr = {mylib.GetBase58Addr(Unpack(accountTbl))}
	assert(TableIsNotEmpty(base58Addr),"GetBase58Addr error")

	local itemsTbl = GetValueFromArray(contract, 74, 4)
	local items = mylib.ByteToInteger(Unpack(itemsTbl))
	assert(items > 0,"items <= 0")

	if #contract ~= 77 + items * 14 then
		error("Betting contract length err.")
	end

	local lType = contract[73]
	local pos = 78


	for i = 1,items do

		local playType = contract[pos]
		local betType = contract[pos + 1]
		local inputTimesTbl = {}
		inputTimesTbl = GetValueFromArray(contract, pos + 2, 4)
		local inputMoneyTbl = {}
		inputMoneyTbl = GetValueFromArray(contract, pos + 6, 8)

		EachBetting(lid, base58Addr, keyser, addr, lType, playType, betType, returnRate, inputTimesTbl, inputMoneyTbl)

		pos = pos + 14
	end

end

--结束投注
function EndBetting()
	local writeDbTbl =
	{
		key="",  --彩票ID
		length = 0,
		value = {}
	}

	--注册的KEY
	local KeySer = {}
	KeySer = GetValueFromArray(contract, 3, 36)
	writeDbTbl.key = serialize(KeySer, false)

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == true,"Not already configured")

	if RegisterTb[70] == 0 then
		error("End betting")
	end

	if RegisterTb[71] == 2 then
		error("The event has been terminated.")
	end

	local accountTbl = {0, 0, 0, 0, 0, 0}
	accountTbl = {mylib.GetCurTxAccount()}
	assert(TableIsNotEmpty(accountTbl),"GetCurTxAccount error")

	local base58Addr = {}
	base58Addr = {mylib.GetBase58Addr(Unpack(accountTbl))}
	assert(TableIsNotEmpty(base58Addr),"GetBase58Addr error")

	local Admin = {}
	Admin = GetValueFromArray(RegisterTb, 1, 34)

	if not MemCmp(Admin, base58Addr,1) == 0 then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Check Admin Account false"),"Check Admin Account false")
		error("Check Admin Account false")
	end

	writeDbTbl.value = RegisterTb;
	writeDbTbl.value[70] = 0 --不可以投注

	writeDbTbl.length = #RegisterTb

	if not mylib.ModifyData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Modify lottery info error"),"Modify lottery info error")
		error("Modify lottery info error")
	end
end


function SaveSession(KeySer, start, length)
	local writeDbTbl =
	{
		key="",  --彩票ID
		length = 0,
		value = {}
	}
	writeDbTbl.key = serialize(KeySer, false)

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == true,"Not already configured")

	local accountTbl = {0, 0, 0, 0, 0, 0}
	accountTbl = {mylib.GetCurTxAccount()}
	assert(TableIsNotEmpty(accountTbl),"GetCurTxAccount error")

	local base58Addr = {}
	base58Addr = {mylib.GetBase58Addr(Unpack(accountTbl))}
	assert(TableIsNotEmpty(base58Addr),"GetBase58Addr error")

	local Admin = {}
	Admin = GetValueFromArray(RegisterTb, 1, 34)

	local platAddr = {}
	platAddr = GetValueFromArray(RegisterTb, 35, 34)

	if not MemCmp(Admin, base58Addr,1) == 0 then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Check Admin Account false"),"Check Admin Account false")
		error("Check Admin Account false")
	end

	--获取返奖率
	local returnRateTbl = {}
	returnRateTbl = GetValueFromArray(RegisterTb, 72, 4)
	local returnRate = mylib.ByteToInteger(Unpack(returnRateTbl))
	assert(returnRate > 0,"returnRate <= 0")

	if RegisterTb[70] ~= 0 then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("The betting isn't over yet"),"The betting isn't over yet")
		error("The betting isn't over yet")
	end

	if RegisterTb[71] == 1 then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Already draw lottery"),"Already draw lottery")
		error("Already draw lottery")
	end

	if RegisterTb[71] == 2 then
		error("The event has been terminated.")
	end

	writeDbTbl.value = RegisterTb
	MemCpy(writeDbTbl.value, start, contract, 40, length)
	writeDbTbl.value[71] = 1

	writeDbTbl.length = #writeDbTbl.value

	if not mylib.ModifyData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Modify lottery info error"),"Modify lottery info error")
		error("Modify lottery info error")
	end

	return returnRate, platAddr

end

function SavePlayInfo(key, rewardIndex)
	local writeDbTbl =
	{
		key="",  --彩票ID
		length = 0,
		value = {}
	}
	writeDbTbl.key = key

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == true,"Not already configured")

	local poolMoneyTbl = {}
	local poolMoney = 0
	poolMoneyTbl = GetValueFromArray(RegisterTb, 39, 8)
	local poolMoney = mylib.ByteToInteger(Unpack(poolMoneyTbl))
	assert(poolMoney >= 0,"poolMoney < 0")

	RegisterTb[38] = rewardIndex

	writeDbTbl.value = RegisterTb
	writeDbTbl.length = #RegisterTb

	if not mylib.ModifyData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Modify info error"),"Modify info error")
		error("Modify info error")
	end

	return poolMoney

end

--抽水到平台地址
function PumpRate(KeySer, platAddr, returnRate, poolMoney)
	local s = serialize(platAddr, false)
	LogPrint(LOG_TYPE.ENUM_STRING,string.len(s),s)
	local moneyTbl = {}
	local money = math.floor((100 - returnRate) / 100 * poolMoney)
	moneyTbl = {mylib.IntegerToByte8(money)}
	SendtoAsset(KeySer, platAddr, moneyTbl)
end

--开奖

--保存字段：管理员地址[34] + 平台地址[34] + 彩种[1] + 是否可以投注[1] +　保留字段[1] + 返奖率后面的字段

--前端下发的数据结构：彩票ID（36字节） 彩种[1字节] (值：1) + 半全场主进球数（4字节） 半全场客进球数（4字节） 半全场比分（10字节） 胜平负结果（1字节） 让胜平负结果（1字节） 总进球结果（1字节）半全场结果（1字节）
					--彩票ID（36字节） 彩种[1字节] (值：2) + 比分（10字节） 胜负结果（1字节） 让分胜负结果（1字节） 胜分差结果（1字节）
function RunLottery()

	--注册的KEY
	local KeySer = {}
	KeySer = GetValueFromArray(contract, 3, 36)
	local lid = {} --彩票ID
	lid = serialize(KeySer, false)

	local lType = contract[39]
	local returnRate = 0
	local poolMoney = 0
	local platAddr = {}
	local totalMoney = 0

	if lType == LOTTERY_TYPE.ENUM_FOOTBALL then
		--保存足彩赛事信息
		returnRate, platAddr = SaveSession(KeySer, 440, 18)

		--胜平负玩法信息
		poolMoney = SavePlayInfo(lid.."01", contract[58])
		totalMoney = totalMoney + poolMoney

		--总进球玩法信息
		poolMoney = SavePlayInfo(lid.."02", contract[59])
		totalMoney = totalMoney + poolMoney

		--单双玩法信息
		poolMoney = SavePlayInfo(lid.."03", contract[60])
		totalMoney = totalMoney + poolMoney

		--半全场玩法信息
		poolMoney = SavePlayInfo(lid.."04", contract[61])
		totalMoney = totalMoney + poolMoney

	elseif lType == LOTTERY_TYPE.ENUM_BASKETBALL then
		--保存篮彩赛事信息
		returnRate, platAddr = SaveSession(KeySer, 440, 10)

		--胜负玩法信息
		poolMoney = SavePlayInfo(lid.."01", contract[50])
		totalMoney = totalMoney + poolMoney

		--让分胜负玩法信息
		poolMoney = SavePlayInfo(lid.."02", contract[51])
		totalMoney = totalMoney + poolMoney

		--总得分玩法信息
		poolMoney = SavePlayInfo(lid.."03", contract[52])
		totalMoney = totalMoney + poolMoney

		--单双玩法信息
		poolMoney = SavePlayInfo(lid.."04", contract[53])
		totalMoney = totalMoney + poolMoney

	else
		error("lottery type error!")
	end

	if totalMoney > 0 then
		PumpRate(KeySer, platAddr, returnRate, totalMoney)
	end
end

function GetRunResult(playId, playNum)
	local writeDbTbl =
	{
		key="",
		length = 0,
		value = {}
	}

	writeDbTbl.key = playId

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == true,"User not configured")

	local rewardIndex = 0
	rewardIndex = RegisterTb[38]
	assert(rewardIndex > 0,"rewardIndex <= 0")
	assert(rewardIndex <= playNum,"rewardIndex > palyNum")

	local oddsIndex = 47 + 8 * playNum + 4 * (rewardIndex - 1)
	local oddsTbl = {}
	oddsTbl = GetValueFromArray(RegisterTb, oddsIndex, 4)
	local odds = mylib.ByteToInteger(Unpack(oddsTbl))
	assert(odds >= 0,"odds <= 0")

	return rewardIndex, odds
end

function SaveBetInfo(betId, rewardIndex, odds)
	local writeDbTbl =
	{
		key="",
		length = 0,
		value = {}
	}

	writeDbTbl.key = betId

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == true,"User not configured")

	assert(RegisterTb[86] == 0,"Already Award")

	local rewardMoneyTbl = {0, 0, 0, 0, 0, 0, 0, 0}
	local rewardMoney = 0
	local win = false

	writeDbTbl.value = RegisterTb
	if rewardIndex == RegisterTb[73] then
		writeDbTbl.value[87] = 1 --中奖标记
		win = true

		local betMoneyTbl = {}
		betMoneyTbl = GetValueFromArray(RegisterTb, 78, 8)
		local betMoney = mylib.ByteToInteger(Unpack(betMoneyTbl))
		assert(betMoney > 0,"betMoney <= 0")

		rewardMoney =  math.floor(betMoney * odds / 100)
		rewardMoneyTbl = {mylib.IntegerToByte8(rewardMoney)}
	else
		writeDbTbl.value[87] = 0
	end

	MemCpy(writeDbTbl.value, 88, rewardMoneyTbl, 1, 8)
	writeDbTbl.value[86] = 1 --是否已经派奖
	writeDbTbl.length = 95

	if not mylib.ModifyData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Modify info error"),"Modify info error")
		error("Modify info error")
	end

	return win, rewardMoneyTbl
end

function EachSendPrize(lid, KeySer, addr, lType, playType, betType)
	local rewardIndex = 0
	local odds = 0

	if lType == LOTTERY_TYPE.ENUM_FOOTBALL then

		if playType == FOOTBALL_PLAY_TYPE.ENUM_SPF then
			--胜平负玩法信息
			rewardIndex, odds = GetRunResult(lid.."01", 3)

		elseif playType == FOOTBALL_PLAY_TYPE.ENUM_TOTAL_NUM then
			--总进球玩法信息
			rewardIndex, odds = GetRunResult(lid.."02", 2)

		elseif playType == FOOTBALL_PLAY_TYPE.ENUM_ODD_EVEN then
			--单双玩法信息
			rewardIndex, odds = GetRunResult(lid.."03", 2)

		elseif playType == FOOTBALL_PLAY_TYPE.ENUM_HALF then
			--半全场玩法信息
			rewardIndex, odds = GetRunResult(lid.."04", 2)
		else
			error("football play type error")
		end
	elseif lType == LOTTERY_TYPE.ENUM_BASKETBALL then

		if playType == BASKETBALL_PLAY_TYPE.ENUM_SF then
			--胜负玩法信息
			rewardIndex, odds = GetRunResult(lid.."01", 2)

		elseif playType == BASKETBALL_PLAY_TYPE.ENUM_RSF then
			--让分胜负玩法信息
			rewardIndex, odds = GetRunResult(lid.."02", 2)

		elseif playType == BASKETBALL_PLAY_TYPE.ENUM_TOTAL_SCORE then
			--总得分玩法信息
			rewardIndex, odds = GetRunResult(lid.."03", 2)

		elseif playType == BASKETBALL_PLAY_TYPE.ENUM_ODD_EVEN then
			--单双玩法信息
			rewardIndex, odds = GetRunResult(lid.."04", 2)

		else
			error("basketball play type error")
		end
	else
		error("lottery type error!")
	end

	local betInfo = {}
	MemCpy(betInfo, 1, KeySer, 1, 36)
	MemCpy(betInfo, 37, addr, 1, 34)
	local info = serialize(betInfo, false)
	info = info .. string.format("%02x",lType)
	info = info .. string.format("%02x",playType)
	info = info .. string.format("%02x",betType)

	local betKeyTbl = {mylib.Sha256(info)}
	local betKey = serialize(betKeyTbl, true)

	--LogPrint(LOG_TYPE.ENUM_STRING,string.len(betKey),betKey)

	local win = false
	local rewardMoneyTbl = {}
	win, rewardMoneyTbl = SaveBetInfo(betKey, rewardIndex, odds)

	if win == true then
		SendtoAsset(KeySer, addr, rewardMoneyTbl)
	end

end

function JudgeRunPrize(lid)

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(lid)
	assert(isRegister == true,"User not configured")

	if RegisterTb[71] == 0 then
		error("Not yet draw the lottery.")
	end

	--assert(RegisterTb[71] == 1,"Not yet draw the lottery.")
	if RegisterTb[71] == 2 then
		error("The event has been terminated.")
	end

end

--派奖
--前端下发的数据结构：彩票ID（36字节） 彩种[1] + 项数（4字节）中奖地址（34字节）玩法（1字节） 投注方案（1字节）......
function SendPrize()

	local KeySer = {}
	KeySer = GetValueFromArray(contract, 3, 36)
	local lid = {}
	lid = serialize(KeySer, false)

	JudgeRunPrize(lid) --判断是否已经开过奖

	local lType = contract[39]
	local itemsTbl = GetValueFromArray(contract, 40, 4)
	local items = mylib.ByteToInteger(Unpack(itemsTbl))
	assert(items > 0,"items <= 0")

	if #contract ~= 43 + items * 36 then
		error("TX_SEND_PRIZE contract length err.")
	end

	local pos = 44

	for i = 1,items do

		local addr = {}
		addr = GetValueFromArray(contract, pos, 34)
		local playType = contract[pos + 34]
		local betType = contract[pos + 35]

		EachSendPrize(lid, KeySer, addr, lType, playType, betType)

		pos = pos + 36
	end

end

function AssertRunLotteryLen(ltype)
	if ltype == LOTTERY_TYPE.ENUM_FOOTBALL then
		assert(#contract == 61, "TX_RUN_LOTTERY contract length err1.")
	elseif ltype ==  LOTTERY_TYPE.ENUM_BASKETBALL then
		assert(#contract == 53, "TX_RUN_LOTTERY contract length err2.")
	else
		assert("lottery type error")
	end
end

function ReturnAsset(fromAddrTbl, toAddrTbl)

  local idTbl =
  {
	idLen = 0,
	idValueTbl = {}
  }
  -- 检查账户中能够转移的数量
  idTbl.idLen = #fromAddrTbl
  idTbl.idValueTbl = fromAddrTbl
  local freeMoneyTbl = {mylib.GetUserAppAccValue(idTbl)}
  assert(TableIsNotEmpty(freeMoneyTbl),"GetUserAppAccValue error")
  local freeMoney = mylib.ByteToInteger(Unpack(freeMoneyTbl))

  if freeMoney == 0 then
  	LogPrint(LOG_TYPE.ENUM_STRING,string.len("The balance of the account is 0."),"The balance of the account is 0.")
	return
  end

  --assert(freeMoney > 0, "The balance of the account is 0.")

  local appOperateTbl = {
		operatorType = 0,  -- 操作类型
		outHeight = 0,    -- 超时高度
		moneyTbl = {},
		userIdLen = 0,    -- 地址长度
		userIdTbl = {},   -- 地址
		fundTagLen = 0,   --fund tag len
		fundTagTbl = {}   --fund tag
	}

  appOperateTbl.operatorType = APP_OPERATOR_TYPE.ENUM_SUB_FREE_OP
  appOperateTbl.userIdLen = idTbl.idLen
  appOperateTbl.userIdTbl = idTbl.idValueTbl
  appOperateTbl.moneyTbl = freeMoneyTbl
  assert(mylib.WriteOutAppOperate(appOperateTbl),"WriteOutAppOperate err1")

  idTbl.idLen = #toAddrTbl
  idTbl.idValueTbl = toAddrTbl
  appOperateTbl.userIdLen = idTbl.idLen
  appOperateTbl.userIdTbl = idTbl.idValueTbl
  appOperateTbl.operatorType = APP_OPERATOR_TYPE.ENUM_ADD_FREE_OP
  assert(mylib.WriteOutAppOperate(appOperateTbl),"WriteOutAppOperate err2")

end

--终止赛事
function Terminate( )
	local writeDbTbl =
	{
		key="",  --彩票ID
		length = 0,
		value = {}
	}

	--注册的KEY
	local KeySer = {}
	KeySer = GetValueFromArray(contract, 3, 36)
	writeDbTbl.key = serialize(KeySer, false)

	local isRegister
	local RegisterTb = {}
	isRegister,RegisterTb = CheckIsConfigKey(writeDbTbl.key)
	assert(isRegister == true,"Not already configured")

	--if RegisterTb[71] == 2 then
	--	error("The event has been terminated.")
	--end

	local accountTbl = {0, 0, 0, 0, 0, 0}
	accountTbl = {mylib.GetCurTxAccount()}
	assert(TableIsNotEmpty(accountTbl),"GetCurTxAccount error")

	local base58Addr = {}
	base58Addr = {mylib.GetBase58Addr(Unpack(accountTbl))}
	assert(TableIsNotEmpty(base58Addr),"GetBase58Addr error")

	local Admin = {}
	Admin = GetValueFromArray(RegisterTb, 1, 34)

	if not MemCmp(Admin, base58Addr,1) == 0 then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Check Admin Account false"),"Check Admin Account false")
		error("Check Admin Account false")
	end

	local platAddr = {}
	platAddr = GetValueFromArray(RegisterTb, 35, 34)

	ReturnAsset(KeySer, platAddr)

	if RegisterTb[71] == 2 then
		return
	end

	writeDbTbl.value = RegisterTb;
	writeDbTbl.value[71] = 2 --终止赛事

	writeDbTbl.length = #RegisterTb

	if not mylib.ModifyData(writeDbTbl) then
		LogPrint(LOG_TYPE.ENUM_STRING,string.len("Modify lottery info error"),"Modify lottery info error")
		error("Modify lottery info error")
	end

end

function Main()
  --[[
  local i = 1

  for i = 1,#contract do
    print("contract")
    print("  ",i,(contract[i]))
  end
  --]]

--[[
  local info = "397FB304-393A-4C10-9142-B1FDCD80B755tpEAv71husxqzpxHF7aRdD95YWTZEa8dqs0101"
  local betKeyTbl = {mylib.Sha256(info)}
  local betKey = serialize(betKeyTbl, true)
  print(betKey)
 --]]

  assert(#contract >= 2,"contract length err.")
  assert(contract[1] == 0xf0,"Contract identification error.")

  local s = string.format("%d", #contract)
  LogPrint(LOG_TYPE.ENUM_STRING,string.len(s),s)

  if contract[2] == TX_TYPE.TX_GAME_CFG then
    assert(#contract > 73,"TX_GAME_CFG contract length err.")
    GameCfg()
  elseif contract[2] ==  TX_TYPE.TX_BET then
    assert(#contract > 87,"TX_BET contract length err.")
	Betting()
  elseif contract[2] ==  TX_TYPE.TX_END_BET then
    assert(#contract == 38,"TX_END_BET contract length err.")
	EndBetting()
  elseif contract[2] ==  TX_TYPE.TX_RUN_LOTTERY then
    assert(#contract >= 39,"TX_RUN_LOTTERY contract length err.")
	AssertRunLotteryLen(contract[39])
	RunLottery()
  elseif contract[2] ==  TX_TYPE.TX_SEND_PRIZE then
    assert(#contract > 43,"TX_SEND_PRIZE contract length err.")
	SendPrize()
  elseif contract[2] ==  TX_TYPE.TX_PUBLISH then
    assert(#contract == 60,"TX_PUBLISH contract length err.")
	Publish()
  elseif contract[2] ==  TX_TYPE.TX_SEND_ASSET then
    assert(#contract == 44,"TX_SEND_ASSET contract length err.")
	Send()
  elseif contract[2] == TX_TYPE.TX_GAME_TERM then
  	assert(#contract == 38, "TX_GAME_TERM contract length err.")
  	Terminate()
  else
    error("RUN_SCRIPT_DATA_ERR")
  end

end

Main()
