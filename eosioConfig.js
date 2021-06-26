module.exports = {
  chains: ['telosTest', 'waxTest', 'jungle3'],
  endpoints: {
    telosTest: ['https://testnet.telos.caleos.io'],
    waxTest: ['https://waxtest.eosn.io'],
    jungle3: ['https://jungle.eosn.io/']
  },
  accountName: {
    telosTest: "ibcbridgedev",
    waxTest: "ibcbridgedev",
    jungle3: "ibcbridgedev",

  },
  contractName: 'bridge',
  cppName: 'bridge'
}