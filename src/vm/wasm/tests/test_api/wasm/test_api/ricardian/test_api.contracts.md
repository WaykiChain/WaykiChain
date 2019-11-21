<h1 class="contract"> printnormal </h1>

---
spec_version: "0.1.0"
title: Create New Token
summary: 'Create a new token'
icon: @ICON_BASE_URL@/@TOKEN_ICON_URI@
---

{{$action.account}} agrees to create a new token with symbol {{asset_to_symbol_code maximum_supply}} to be managed by {{issuer}}.

This action will not result any any tokens being issued into circulation.

{{issuer}} will be allowed to issue tokens into circulation, up to a maximum supply of {{maximum_supply}}.


<h1 class="contract"> print1 </h1>

---
spec_version: "0.1.0"
title: Issue Tokens into Circulation
summary: 'Issue {{nowrap quantity}} into circulation and transfer into {{nowrap to}}’s account'
icon: @ICON_BASE_URL@/@TOKEN_ICON_URI@
---

The token manager agrees to issue {{quantity}} into circulation, and transfer it into {{to}}’s account.

{{#if memo}}There is a memo attached to the transfer stating:
{{memo}}
{{/if}}


<h1 class="contract"> print2 </h1>

---
spec_version: "0.1.0"
title: Open Token Balance
summary: 'Open a zero quantity balance for {{nowrap owner}}'
icon: @ICON_BASE_URL@/@TOKEN_ICON_URI@
---

{{ram_payer}} agrees to establish a zero quantity balance for {{owner}} for the {{symbol_to_symbol_code symbol}} token.

If {{owner}} does not have a balance for {{symbol_to_symbol_code symbol}}, {{ram_payer}} will be designated as the RAM payer of the {{symbol_to_symbol_code symbol}} token balance for {{owner}}.


<h1 class="contract"> print3 </h1>

---
spec_version: "0.2.0"
title: Transfer Tokens
summary: 'Send {{nowrap quantity}} from {{nowrap from}} to {{nowrap to}}'
icon: @ICON_BASE_URL@/@TRANSFER_ICON_URI@
---

{{from}} agrees to send {{quantity}} to {{to}}.

{{#if memo}}There is a memo attached to the transfer stating:
{{memo}}
{{/if}}


<h1 class="contract"> print4 </h1>

---
spec_version: "0.1.0"
title: Open Token Balance
summary: 'Open a zero quantity balance for {{nowrap owner}}'
icon: @ICON_BASE_URL@/@TOKEN_ICON_URI@
---

{{ram_payer}} agrees to establish a zero quantity balance for {{owner}} for the {{symbol_to_symbol_code symbol}} token.


<h1 class="contract"> print5 </h1>

---
spec_version: "0.2.0"
title: Close Token Balance
summary: 'Close {{nowrap owner}}’s zero quantity balance'
icon: @ICON_BASE_URL@/@TOKEN_ICON_URI@
---

{{owner}} agrees to close their zero quantity balance for the {{symbol_to_symbol_code symbol}} token.