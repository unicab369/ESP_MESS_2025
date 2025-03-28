


## When AES Alone is Not Enough

| Scenario	        | Risk	                    | Solution  |
|-------------------|---------------------------|-----------|
| Replay attacks	| Hacker re-sends old data	| Add a timestamp/nonce + sequence number.
| Tampering	        | Data altered in transit	| Use AES-GCM (encryption + integrity) or AES + HMAC-SHA256. |
| Key leakage	    | Long-term key compromised	| Rotate keys or use asymmetric encryption (ECDH) for key exchange. |


nonce: "number used once"