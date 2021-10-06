# CHANGELOG

<!--- next entry here -->

## 0.11.2
2021-04-08

### API
- Add `QLIB_GetResetStatus` function to read the last reset reason (status). New function - **Backwards compatible**.
- Add `QLIB_GetHwVersion` function to read the flash version. New function - **Backwards compatible**.
- Change `QLIB_GetId` to include only the unique ID. **Not Backwards compatible**.
- `QLIB_ConfigDevice`: Change `QLIB_DEVICE_CONF_T` parameter structure to include standard address size configuration option. **Not backwards compatible**.
- `QLIB_IsMaintenanceNeeded` deprecated and added `QLIB_GetNotifications` instead in order to return more information from W77Q. **Not backwards compatible**.
- `CORE_RESET`: changed to a function rather than macro. Need to change the implementation in qlib_platform.c. **Not backwards compatible**.
- `PLAT_SHA`: changed to `PLAT_HASH` as QLIB now support multiple hash functions. **Not backwards compatible**.

### Features

- QCONF: Change QCONF to support configuration of sections with CRC and Digest when there is no direct flash access.
- Change samples to use different keys values for restricted and full access key of each section

### Fixes

- `QLIB_Format` - Added SW reset after format
- After review of the Security flows described in the HW spec - adjust QLIB to behave as described in the Security flows
- Add W77Q128 support
- Updated QLIB manual documentation
  - Add optimization section to quick start guide
  - `QLIB_ConfigSection` documentation now states that permission access policy of section 0 and 7 shall be identical
- Authenticated PA revoke does not close the active session
- Several bug fixes

### Performance results

|                             | NXP-1050<br>Secure OP<br>usec | NXP-1050<br>Secure OP<br>MB/s     | NXP-1050<br>Standard OP<br>usec | NXP-1050<br>Standard OP<br>MB/s | NXP-LPC54005<br>Secure OP<br>usec | NXP-LPC54005<br>Secure OP<br>MB/s | NXP-LPC54005<br>Standard OP<br>usec | NXP-LPC54005<br>Standard OP<br>MB/s |
|-----------------------------|-------------------------------|-----------------------------------|---------------------------------|---------------------------------|-----------------------------------|-----------------------------------|-------------------------------------|-------------------------------------|
| Read SDR Single(256 bytes)  |              75               |               3.413               |                25               |              10.240             |               84                  |               3.048               |                  27                 |                9.481                |
| Read SDR Quad(256 bytes)    |              62               |               4.129               |                14               |              18.286             |               63                  |               4.063               |                  9                  |                28.444               |
| Read DTR Quad(256 bytes)    |              62               |               4.129               |                14               |              18.286             |                                   |                                   |                                     |                                     |
| Write SDR Single(256 bytes) |              863              |               0.297               |               550               |              0.465              |              826                  |               0.310               |                 508                 |                0.504                |
| Write SDR Quad(256 bytes)   |              843              |               0.304               |               537               |              0.477              |              801                  |               0.320               |                 492                 |                0.520                |
| Erase Sector Single         |             74095             |               0.003               |              72200              |              0.004              |             68900                 |               0.004               |                68881                |                0.004                |
| Erase 32K Block Single      |             136525            |               0.002               |              136833             |              0.002              |             123547                |               0.002               |                130870               |                0.002                |
| Erase 64K Block Single      |             21340             |               0.012               |              204772             |              0.001              |             198274                |               0.001               |                198860               |                0.001                |
| Erase Sector Quad           |             72059             |               0.004               |              72154              |              0.004              |             70627                 |               0.004               |                68812                |                0.004                |
| Erase 32K Block Quad        |             136345            |               0.002               |              136763             |              0.002              |             129889                |               0.002               |                135625               |                0.002                |
| Erase 64K Block Quad        |             220932            |               0.001               |              214004             |              0.001              |             206956                |               0.001               |                183414               |                0.001                |

Environment used:
- Secure code is running from RAM

- SPI frequency:
  - NXP 1050 - 100 MHz
  - NXP LPC54005 - 90 MHz

- CPU frequency:
  - NXP 1050 - 600 MHz
  - NXP LPC54005 - 180 MHz

- SHA:
  - NXP 1050 - Using HW SHA engine in the SoC
  - NXP LPC54005 - Using HW SHA engine in the SoC

## 0.11.1
2021-04-08

**Internal only**

## 0.11.0
2021-04-08

**Internal only**

## 0.10.1
2021-01-12

### Performance results
**Same as 0.10.0 below**

### Fixes
- Double getSSR in dual (1_1_2) mode fix

## 0.10.0
2021-01-07

### Performance results

|                             | NXP-1050<br>Secure OP<br>usec | NXP-1050<br>Secure OP<br>MB/s     | NXP-1050<br>Standard OP<br>usec | NXP-1050<br>Standard OP<br>MB/s | NXP-LPC54005<br>Secure OP<br>usec | NXP-LPC54005<br>Secure OP<br>MB/s | NXP-LPC54005<br>Standard OP<br>usec | NXP-LPC54005<br>Standard OP<br>MB/s |
|-----------------------------|-------------------------------|-----------------------------------|---------------------------------|---------------------------------|-----------------------------------|-----------------------------------|-------------------------------------|-------------------------------------|
| Read SDR Single(256 bytes)  |              105              |               2.438               |                27               |              9.481              |               94                  |               2.723               |                  28                 |                9.143                |
| Read SDR Quad(256 bytes)    |              75               |               3.413               |                14               |              18.286             |               62                  |               4.129               |                  9                  |                28.444               |
| Read DTR Quad(256 bytes)    |              90               |               2.844               |                16               |              16.000             |                                   |                                   |                                     |                                     |
| Write SDR Single(256 bytes) |              889              |               0.288               |               555               |              0.461              |              867                  |               0.295               |                 544                 |                0.471                |
| Write SDR Quad(256 bytes)   |              851              |               0.301               |               539               |              0.475              |              826                  |               0.310               |                 524                 |                0.489                |
| Erase Sector Single         |             64337             |               0.004               |              64337              |              0.004              |             60001                 |               0.004               |                59716                |                0.004                |
| Erase 32K Block Single      |             137732            |               0.002               |              137218             |              0.002              |             131165                |               0.002               |                133566               |                0.002                |
| Erase 64K Block Single      |             223840            |               0.001               |              223553             |              0.001              |             200433                |               0.001               |                205713               |                0.001                |
| Erase Sector Quad           |             67411             |               0.004               |              65808              |              0.004              |             62242                 |               0.004               |                62197                |                0.004                |
| Erase 32K Block Quad        |             136132            |               0.002               |              135846             |              0.002              |             145464                |               0.002               |                139700               |                0.002                |
| Erase 64K Block Quad        |             208681            |               0.001               |              208474             |              0.001              |             225029                |               0.001               |                215920               |                0.001                |

Environment used:
- Secure code is running from RAM

- SPI frequency:
  - NXP 1050 - 100 MHz
  - NXP LPC54005 - 90 MHz

- CPU frequency:
  - NXP 1050 - 600 MHz
  - NXP LPC54005 - 180 MHz

- SHA:
  - NXP 1050 - Using HW SHA engine in the SoC
  - NXP LPC54005 - Using HW SHA engine in the SoC

### Features
- Allow key_SECRET to be NULL

### Other changes
- Additional Secure Read performance enhancements for NXP platform
- Double getSSR in single(1_1_1) mode fix


## 0.9.1
2020-12-28

### Performance results

|                             | NXP-1050<br>Secure OP<br>usec | NXP-1050<br>Secure OP<br>MB/s     | NXP-1050<br>Standard OP<br>usec | NXP-1050<br>Standard OP<br>MB/s | NXP-LPC54005<br>Secure OP<br>usec | NXP-LPC54005<br>Secure OP<br>MB/s | NXP-LPC54005<br>Standard OP<br>usec | NXP-LPC54005<br>Standard OP<br>MB/s |
|-----------------------------|-------------------------------|-----------------------------------|---------------------------------|---------------------------------|-----------------------------------|-----------------------------------|-------------------------------------|-------------------------------------|
| Read SDR Single(256 bytes)  |              88               |               2.909               |                25               |              10.240             |               82                  |               3.122               |                  26                 |                9.846                |
| Read SDR Quad(256 bytes)    |              74               |               3.459               |                14               |              18.286             |               61                  |               4.197               |                  9                  |                28.444               |
| Read DTR Quad(256 bytes)    |              90               |               2.844               |                16               |              16.000             |                                   |                                   |                                     |                                     |
| Write SDR Single(256 bytes) |              886              |               0.289               |               561               |              0.456              |              902                  |               0.284               |                 577                 |                0.444                |
| Write SDR Quad(256 bytes)   |              865              |               0.296               |               548               |              0.467              |              875                  |               0.293               |                 555                 |                0.461                |
| Erase Sector Single         |             41001             |               0.006               |              43862              |              0.006              |             48540                 |               0.005               |                50192                |                0.005                |
| Erase 32K Block Single      |             97457             |               0.003               |              98754              |              0.003              |             102092                |               0.003               |                100887               |                0.003                |
| Erase 64K Block Single      |             135307            |               0.002               |              170589             |              0.002              |             171214                |               0.001               |                181329               |                0.001                |
| Erase Sector Quad           |             43769             |               0.006               |              42366              |              0.006              |             46995                 |               0.005               |                49774                |                0.005                |
| Erase 32K Block Quad        |             114773            |               0.002               |              98026              |              0.003              |             101788                |               0.003               |                101151               |                0.003                |
| Erase 64K Block Quad        |             183457            |               0.001               |              185585             |              0.001              |             169927                |               0.002               |                169861               |                0.002                |

Environment used:
- Secure code is running from RAM

- SPI frequency:
  - NXP 1050 - 100 MHz
  - NXP LPC54005 - 90 MHz

- CPU frequency:
  - NXP 1050 - 600 MHz
  - NXP LPC54005 - 180 MHz

- SHA:
  - NXP 1050 - Using HW SHA engine in the SoC
  - NXP LPC54005 - Using HW SHA engine in the SoC
  
### Fixes

- Update the WD LF-oscillator calibration value
- Remove multi transaction optimization from QLIB_write

## 0.9.0
2020-12-21

### Performance results

|                             | NXP-1050<br>Secure OP<br>usec | NXP-1050<br>Secure OP<br>MB/s     | NXP-1050<br>Standard OP<br>usec | NXP-1050<br>Standard OP<br>MB/s | NXP-LPC54005<br>Secure OP<br>usec | NXP-LPC54005<br>Secure OP<br>MB/s | NXP-LPC54005<br>Standard OP<br>usec | NXP-LPC54005<br>Standard OP<br>MB/s |
|-----------------------------|-------------------------------|-----------------------------------|---------------------------------|---------------------------------|-----------------------------------|-----------------------------------|-------------------------------------|-------------------------------------|
| Read SDR Single(256 bytes)  |              88               |               2.909               |                25               |              10.240             |               82                  |               3.122               |                  27                 |                9.481                |
| Read SDR Quad(256 bytes)    |              74               |               3.459               |                14               |              18.286             |               61                  |               4.197               |                  9                  |                28.444               |
| Read DTR Quad(256 bytes)    |              90               |               2.844               |                16               |              16.000             |                                   |                                   |                                     |                                     |
| Write SDR Single(256 bytes) |              886              |               0.289               |               561               |              0.456              |              863                  |               0.297               |                 554                 |                0.462                |
| Write SDR Quad(256 bytes)   |              858              |               0.298               |               548               |              0.467              |              839                  |               0.305               |                 536                 |                0.478                |
| Erase Sector Single         |             43880             |               0.006               |              43921              |              0.006              |             43486                 |               0.006               |                42093                |                0.006                |
| Erase 32K Block Single      |             91379             |               0.003               |              92057              |              0.003              |             91105                 |               0.003               |                87137                |                0.003                |
| Erase 64K Block Single      |             175474            |               0.001               |              177478             |              0.001              |             157901                |               0.002               |                158800               |                0.002                |
| Erase Sector Quad           |             45394             |               0.006               |              45419              |              0.006              |             42017                 |               0.006               |                42020                |                0.006                |
| Erase 32K Block Quad        |             86783             |               0.003               |              87732              |              0.003              |             90503                 |               0.003               |                91089                |                0.003                |
| Erase 64K Block Quad        |             191827            |               0.001               |              177376             |              0.001              |             156319                |               0.002               |                174500               |                0.001                |

Environment used:
- Secure code is running from RAM

- SPI frequency:
  - NXP 1050 - 100 MHz			
  - NXP LPC54005 - 90 MHz			

- CPU frequency:
  - NXP 1050 - 600 MHz		
  - NXP LPC54005 - 180 MHz			

- SHA:
  - NXP 1050 - Using HW SHA engine in the SoC			
  - NXP LPC54005 - Using HW SHA engine in the SoC
  
### API

- Added the following API functions:
  - `QLIB_STATUS_T QLIB_AuthPlainAccess_Grant(QLIB_CONTEXT_T* qlibContext, U32 sectionID)`
  - `QLIB_STATUS_T QLIB_AuthPlainAccess_Revoke(QLIB_CONTEXT_T* qlibContext, U32 sectionID)`
  - `QLIB_STATUS_T QLIB_PlainAccessEnable(QLIB_CONTEXT_T* qlibContext, U32 sectionID)`
  - `QLIB_STATUS_T QLIB_CalcCDI(QLIB_CONTEXT_T* qlibContext, _256BIT nextCdi, _256BIT prevCdi, U32 sectionId)`
  - `QLIB_STATUS_T QLIB_Watchdog_ConfigGet(QLIB_CONTEXT_T* qlibContext, QLIB_WATCHDOG_CONF_T* watchdogCFG)`
  - `QLIB_STATUS_T QLIB_SetActiveChip(QLIB_CONTEXT_T* qlibContext, U8 die)`  

- Removed the following API functions:
  - `QLIB_ConfigSectionTable`
  - `QLIB_ConfigInterface`

- Renamed the following API functions:
  - `QLIB_GetSectionConfigurations` to `QLIB_GetSectionConfiguration`
  - `QLIB_Watchdog_Config` to `QLIB_Watchdog_ConfigSet`

- Added parameters to the following API functions:
  - `QLIB_Watchdog_Get` - `U32* ticsResidue`

### Features

- LS bits randomization in Read/Write/Erase secure commands
- Added SW Get_NONCE, and SHA to the platform
- QCONF supports 64 bit core address
- Change chip name from w77m_w25q256jw to w77m32i
- Performance improvements (INTRLV_EN, resp_ready, multi transaction command, aligned read, memcpy optimizations)


### Fixes

- errata SEC-5,SSR-5 workarounds
- disable checksumIntegrity from the QCONF configuration in the sample code
- WD calibration has overwritten at the follow POR
- enable Speculative Cypher Key Generation
- bug in SRD optimization that makes MC get out of sync
- GET_SSR is forbidden in power-down mode
- set active chip to die 0 on connect/disconnect for server synch, and check connection before STD commands

### Other changes

- adding documentation for QLIB_ConfigSection limitations when XIP 

## 0.8.0
2020-10-20


### API changes

- adding API to grant and revoke authenticated plain access 

### Features
- multi-die support     

### Fixes

- **performance:** adding inline functions, optimization for multi-transaction reads/writes
- **code:** fix bug in QLIB_SAMPLE_WatchDogCalibrate()

### Other changes
- Added documentation for security flows

## 0.7.0
2020-08-26

### Performance results

|                             | NXP-1050<br>Secure OP<br>usec | NXP-1050<br>Secure OP<br>MB/s | NXP-1050<br>Standard OP<br>usec | NXP-1050<br>Standard OP<br>MB/s | NXP-LPC54005<br>Secure OP<br>usec | NXP-LPC54005<br>Secure OP<br>MB/s | NXP-LPC54005<br>Standard OP<br>usec | NXP-LPC54005<br>Standard OP<br>MB/s |
|-----------------------------|-------------------------------|-----------------------------------|---------------------------------|---------------------------------|-------------------------------|-----------------------------------|-------------------------------------|-------------------------------------|
| Read SDR Single(256 bytes)  |              156              |               1.641               |                28               |              9.143              |              114              |               2.246               |                  25                 |                10.24                |
| Read SDR Quad(256 bytes)    |              146              |               1.753               |                22               |              11.636             |               95              |               2.695               |                  9                  |                28.444               |
| Read DTR Quad(256 bytes)    |              146              |               1.753               |                22               |              11.636             |                               |                                   |                                     |                                     |
| Write SDR Single(256 bytes) |              882              |                0.29               |               517               |              0.495              |              841              |               0.304               |                 501                 |                0.511                |
| Write SDR Quad(256 bytes)   |              887              |               0.292               |               506               |              0.506              |              814              |               0.314               |                 485                 |                0.528                |
| Erase Sector Single         |             51611             |               0.005               |              50452              |              0.005              |             49864             |               0.005               |                49857                |                0.005                |
| Erase 32K Block Single      |             100265            |               0.003               |              102819             |              0.002              |             97214             |               0.003               |                97257                |                0.003                |
| Erase 64K Block Single      |             164007            |               0.002               |              170082             |              0.002              |             174368            |               0.001               |                169074               |                0.002                |
| Erase Sector Quad           |             51612             |               0.005               |              52772              |              0.005              |             51005             |               0.005               |                50985                |                0.005                |
| Erase 32K Block Quad        |             102404            |               0.002               |              100316             |              0.003              |             97068             |               0.003               |                99358                |                0.003                |
| Erase 64K Block Quad        |             173893            |               0.001               |              169516             |              0.002              |             162729            |               0.002               |                163204               |                0.002                |

Environment used:
- Secure code is running from RAM

- SPI frequency:
  - NXP 1050 - 100 MHz			
  - NXP LPC54005 - 90 MHz			

- CPU frequency:
  - NXP 1050 - 600 MHz		
  - NXP LPC54005 - 180 MHz			

- SHA:
  - NXP 1050 - Using HW SHA engine in the SoC			
  - NXP LPC54005 - Using HW SHA engine in the SoC	

### Features

- **code:** Move QPI code under `QLIB_SUPPORT_QPI` definition.
- **performance:** Improve performance by adding "idle task" feature to the TM layer. This allows to perform tasks in parallel to the flash operation, while the flash is busy.

### Fixes

- **performance:** Improve performance by:
  - Set reserved bit (INTRLV_EN) to 0 in the device configuration function
  - Improving `QLIB_CRYPTO` functions
  - Create `QLIB_SEC_READ_DATA_T` type which reduce the number of memcpy used
  - Remove the "get SSR" after "OP2" of every secure command as "OP2" cannot trigger any security related errors
  - Error masks now include only the error bits and bypass all the specific bits check in `QLIB_CMD_PROC__checkLastSsrErrors()` if no error is found
- **samples:**
  - WD calibration, - fix a bug where the calibration value did not update the permanent WD configuration register. Therefore WD calibration has overwritten at the follow POR
  - Enable Speculative Cipher Key Generation for better performance
  - Disable `checksumIntegrity` from the QCONF configuration in the sample code
- **code:** add support for 64Bit core/address to QCONF
- **code:** fix bug in `QLIB_InitDevice()` to prevent from the function to fail
- **documentation:** add `QLIB_SUPPORT_QPI` definition to quick_start.md

### Known Issues

- not support QPI while executing from flash (XIP)
- suspend/resume mechanism is not fully supported

## 0.6.1
2020-08-04

### Fixes

- **qconf:** Define `QCONF_NO_DIRECT_FLASH_ACCESS` to allows QCONF to run when there is no direct access to the flash. This mode has two effects:
  - The configuration structure will not be erased (keys included) 
  - Integrity protected sections are not supported

## 0.6.0
2020-06-25

### API changes

- remove INTRLV_EN configuration from QLIB_DEVICE_CONF_T structure passes to API function QLIB_ConfigDevice.
- QLIB_GetSectionConfigurations returns size sero for disabled section 
- replace QLIB_BUS_MODE_T and DTR with QLIB_BUS_FORMAT_T in QLIB_InitDevice, QLIB_SetInterface, QLIB_ConfigInterface APIs
- replace key pointers arrays restrictedKeys and fullAccessKeys in QLIB_ConfigDevice API with key arrays
- replace resetResp pointer in QLIB_DEVICE_CONF_T structure passes to API function QLIB_ConfigDevice resetResp structure.
- Now QLIB_ConfigDevice() allows to receive valid keys & SUID ans not return error if keys already provisioned

### Features

- **qconf:** add qconf code, samples and documentation

### Fixes

- **code:** Allow to execute "SET_SCR/SWAP" based APIs from XIP
- **documentation:** add license agreement text file
- **qpi:** fix reset_flash command in QPI mode
- **documentation:** Updating to correct flow-chart
- **documentation:** added a list of macros that must be linked to RAM


## 0.5.2
2020-06-04

### Fixes

- **documentation:** added software stack drawing and RAM usage indication 

## 0.5.1
2020-06-01

### Fixes

- **documentation:** add quick start page
- **documentation:** added interface variables boundaries indication in qlib.h
- **samples:** update sample code for fw update and watchdog
 

## 0.5.0
2020-05-26

### API changes

- remove the API function QLIB_EraseChip().
- add "eraseDataOnly" parameter to QLIB_Format() that if TRUE then SERASE (Secure Erase) command is used to securely erase only the data of all sections, configurations and keys do not change in this case.
- standard QLIB operations return specific error instead of general QLIB_STATUS__DEVICE_ERR

### Features

- **documentation:** updated QLIB documentation

### Fixes

- **code:** improve errors return from QLIB_Erase API
- **code:** fix the QPI support for secure commands

### Known Issues
- suspend/resume mechanism is not fully supported

## 0.4.0
2020-05-13

### Features

- **api:** add QLIB_SetInterface API to change bus format without changing the flash configurations
- **samples:** add sample code for: device configuration, fw update, secure storage, authenticated watchdog
- **doc:** Adding improved doxygen based user-guide (preliminary version)

### Known Issues
- suspend/resume mechanism is not fully supported
- QPI mode is not supported
- Erasing rollback protected section with CHIP_ERASE or ERASE_SECT_PA commands erases the active part instead of the inactive part of the section

## 0.3.0
2020-04-30

### Features

- **configuration** QLIB_SEC_ONLY pre-compile configuration to suppress all legacy support from qlib
- **api** added QLIB_GetStatus API
- **code** allows to config the buss interface after QLIB_InitLib and before QLIB_InitDevice

### Fixes

- **documentation:**
- **code** improve QLIB_STD_autoSense_L - check for single format first and replace getId command with more efficient get JEDEC command
- **code** do not reset the flash at initDevice, instead resume any existing erase/write and wait for the device to be ready
- **code** remove QLIB_STD_CheckDeviceIsBusy_L before STD commands
- **code** allows QLIB_TM_Standard accept NULL status pointer

## 0.2.0
2020-04-22

### Features

- **api:** Add expire parameter to QLIB_Watchdog_Get API
- **sec:** Add SSR state field definitions
- **all:** Add API to use logical address instead of sectionID/offset pair
- **api:** Add support for secure/non-secure Section Erase

### Fixes

- **sim:** Fix flash file handling
- **deliverables:** Add src directory, Add PDF document and optimize doxygen
- **documentation:** Add license information
- **doc:** Fix doxygen errors
- **suspend/resume:** Fix suspend/resume state checks
- **all:** Fix packed structures handling
- **sec:** Read watchdog configuration during Init
- **xip:** Do not disable quad mode to allows for quad XIP

## 0.1.0
2020-04-07

### First released Version  
