# NB-IoT vs LTE-M Comparison

## 1. Technology Basics
| Feature          | NB-IoT                          | LTE-M                          |
|------------------|---------------------------------|--------------------------------|
| **Standard**     | 3GPP Release 13                | 3GPP Release 13               |
| **Bandwidth**    | 180kHz (Single narrowband)      | 1.4MHz (Wider than NB-IoT)    |
| **Modulation**   | OFDMA (Downlink), SC-FDMA (Uplink) | Same as LTE (QPSK/16QAM)     |

## 2. Performance
| Metric           | NB-IoT                          | LTE-M                          |
|------------------|---------------------------------|--------------------------------|
| **Data Rate**    | ~50-250kbps (Slow)              | ~1Mbps (Faster)               |
| **Latency**      | 1.6-10s (Higher)                | 50-100ms (Lower)              |
| **Range**        | Excellent (Deep indoor)         | Good (Better than LTE)        |
| **Mobility**     | None (Static devices)           | Supports handovers (Mobile)   |

## 3. Power Consumption
| Scenario        | NB-IoT                     | LTE-M                     |
|----------------|----------------------------|---------------------------|
| **PSM Mode**   | ~5µA (Ultra-low power)     | ~5µA                     |
| **eDRX Cycle** | Up to 3hrs (Long sleep)    | Up to 43min              |
| **Battery Life**| 10+ years (Optimized)      | 5-10 years               |

## 4. Deployment
| Type            | NB-IoT                          | LTE-M                          |
|------------------|---------------------------------|--------------------------------|
| **Spectrum**     | In-band LTE, Guard-band, Standalone | Uses existing LTE bands      |
| **Global Use**   | China/Europe dominant          | North America/Global          |
| **Cost**         | Lower (Simpler hardware)       | Slightly higher              |

## 5. Use Cases
| NB-IoT                          | LTE-M                          |
|----------------------------------|--------------------------------|
| - Smart meters (water/gas)       | - Fleet tracking               |
| - Environmental sensors          | - Wearables                    |
| - Agricultural monitoring        | - Smart city infrastructure    |
| - Parking sensors                | - Healthcare devices           |

## Key Takeaways
- **Choose NB-IoT when**:
  - Need deep indoor penetration
  - Ultra-low power is critical
  - Device is static (no mobility)
  - Data requirements are minimal

- **Choose LTE-M when**:
  - Higher data rates needed
  - Mobility support required
  - Voice support needed (VoLTE)
  - Lower latency is critical

> **Note**: Both are LPWAN technologies under the 3GPP standard, but serve different market needs.