/* MIT License
 *
 * Copyright (c) 2017 GrieferAtWork
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __KOS_KERNEL_DEV_STORAGE_ATA_H__
#define __KOS_KERNEL_DEV_STORAGE_ATA_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/types.h>
#include <kos/errno.h>
#include <kos/kernel/dev/storage.h>

__DECL_BEGIN

#define /*katabus_t*/ATA_IOPORT_PRIMARY   0x1F0 /*< ... 0x1F7 */
#define /*katabus_t*/ATA_IOPORT_SECONDARY 0x170 /*< ... 0x177 */

// Values for the 'ATA_IOPORT_HDDEVSEL' command
#define /*katadrive_t*/ATA_DRIVE_MASTER  0xA0
#define /*katadrive_t*/ATA_DRIVE_SLAVE   0xB0

#define ATA_IOPORT_DATA(bus)       ((bus)+0x00)
#define ATA_IOPORT_ERROR(bus)      ((bus)+0x01)
#define ATA_IOPORT_FEATURES(bus)   ((bus)+0x01)
#define ATA_IOPORT_SECCOUNT0(bus)  ((bus)+0x02)
#define ATA_IOPORT_LBALO(bus)      ((bus)+0x03)
#define ATA_IOPORT_LBAMD(bus)      ((bus)+0x04)
#define ATA_IOPORT_LBAHI(bus)      ((bus)+0x05)
#define ATA_IOPORT_HDDEVSEL(bus)   ((bus)+0x06)
#define ATA_IOPORT_COMMAND(bus)    ((bus)+0x07)
#define ATA_IOPORT_STATUS(bus)     ((bus)+0x07)
#define ATA_IOPORT_SECCOUNT1(bus)  ((bus)+0x08)
#define ATA_IOPORT_LBA3(bus)       ((bus)+0x09)
#define ATA_IOPORT_LBA4(bus)       ((bus)+0x0A)
#define ATA_IOPORT_LBA5(bus)       ((bus)+0x0B)
#define ATA_IOPORT_CONTROL(bus)    ((bus)+0x0C)
#define ATA_IOPORT_ALTSTATUS(bus)  ((bus)+0x0C)
#define ATA_IOPORT_DEVADDRESS(bus) ((bus)+0x0D)

//////////////////////////////////////////////////////////////////////////
// Commands used by the 'ATA_IOPORT_COMMAND' ioport
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC

//////////////////////////////////////////////////////////////////////////
// Flags returned by the 'ATA_IOPORT_STATUS' ioport
#define ATA_STATUS_ERR   0x01 // 1 << 0
#define ATA_STATUS_IDX   0x02 // 1 << 1
#define ATA_STATUS_CORR  0x04 // 1 << 2
#define ATA_STATUS_DRQ   0x08 // 1 << 3
#define ATA_STATUS_DSC   0x10 // 1 << 4
#define ATA_STATUS_DF    0x20 // 1 << 5
#define ATA_STATUS_DRDY  0x40 // 1 << 6
#define ATA_STATUS_BSY   0x80 // 1 << 7


__COMPILER_PACK_PUSH(1)

struct __packed ata_generalconfiguration {
 unsigned int Reserved1 : 1;
 unsigned int Retired3 : 1;
 unsigned int ResponseIncomplete : 1;
 unsigned int Retired2 : 3;
 unsigned int FixedDevice : 1;
 unsigned int RemovableMedia : 1;
 unsigned int Retired1 : 7;
 unsigned int DeviceType : 1;
};

struct __packed ata_capabilities {
 __u8  ReservedByte49;
 unsigned int DmaSupported : 1;
 unsigned int LbaSupported : 1;
 unsigned int IordyDisable : 1;
 unsigned int IordySupported : 1;
 unsigned int Reserved1 : 1;
 unsigned int StandybyTimerSupport : 1;
 unsigned int Reserved2 : 2;
 __u16 ReservedWord50;
};

struct __packed ata_commandset {
 unsigned int SmartCommands : 1;
 unsigned int SecurityMode : 1;
 unsigned int RemovableMediaFeature : 1;
 unsigned int PowerManagement : 1;
 unsigned int Reserved1 : 1;
 unsigned int WriteCache : 1;
 unsigned int LookAhead : 1;
 unsigned int ReleaseInterrupt : 1;
 unsigned int ServiceInterrupt : 1;
 unsigned int DeviceReset : 1;
 unsigned int HostProtectedArea : 1;
 unsigned int Obsolete1 : 1;
 unsigned int WriteBuffer : 1;
 unsigned int ReadBuffer : 1;
 unsigned int Nop : 1;
 unsigned int Obsolete2 : 1;
 unsigned int DownloadMicrocode : 1;
 unsigned int DmaQueued : 1;
 unsigned int Cfa : 1;
 unsigned int AdvancedPm : 1;
 unsigned int Msn : 1;
 unsigned int PowerUpInStandby : 1;
 unsigned int ManualPowerUp : 1;
 unsigned int Reserved2 : 1;
 unsigned int SetMax : 1;
 unsigned int Acoustics : 1;
 unsigned int BigLba : 1;
 unsigned int DeviceConfigOverlay : 1;
 unsigned int FlushCache : 1;
 unsigned int FlushCacheExt : 1;
 unsigned int Resrved3 : 2;
 unsigned int SmartErrorLog : 1;
 unsigned int SmartSelfTest : 1;
 unsigned int MediaSerialNumber : 1;
 unsigned int MediaCardPassThrough : 1;
 unsigned int StreamingFeature : 1;
 unsigned int GpLogging : 1;
 unsigned int WriteFua : 1;
 unsigned int WriteQueuedFua : 1;
 unsigned int WWN64Bit : 1;
 unsigned int URGReadStream : 1;
 unsigned int URGWriteStream : 1;
 unsigned int ReservedForTechReport : 2;
 unsigned int IdleWithUnloadFeature : 1;
 unsigned int Reserved4 : 2;
};

struct __packed ata_sectorsize {
 unsigned int LogicalSectorsPerPhysicalSector : 4;
 unsigned int Reserved0 : 8;
 unsigned int LogicalSectorLongerThan256Words : 1;
 unsigned int MultipleLogicalSectorsPerPhysicalSector : 1;
 unsigned int Reserved1 : 2;
};
struct __packed ata_commandsetext {
 unsigned int ReservedForDrqTechnicalReport : 1;
 unsigned int WriteReadVerifyEnabled : 1;
 unsigned int Reserved01 : 11;
 unsigned int Reserved1 : 2;
};
struct __packed ata_securitystatus {
 unsigned int SecuritySupported : 1;
 unsigned int SecurityEnabled : 1;
 unsigned int SecurityLocked : 1;
 unsigned int SecurityFrozen : 1;
 unsigned int SecurityCountExpired : 1;
 unsigned int EnhancedSecurityEraseSupported : 1;
 unsigned int Reserved0 : 2;
 unsigned int SecurityLevel : 1;
 unsigned int Reserved1 : 7;
};
struct __packed ata_cfapowermodel {
 unsigned int MaximumCurrentInMA2 : 12;
 unsigned int CfaPowerMode1Disabled : 1;
 unsigned int CfaPowerMode1Required : 1;
 unsigned int Reserved0 : 1;
 unsigned int Word160Supported : 1;
};
struct __packed ata_datasetmanagementfeature {
 unsigned int SupportsTrim : 1;
 unsigned int Reserved0 : 15;
};
struct __packed ata_blockalignment {
 unsigned int AlignmentOfLogicalWithinPhysical : 14;
 unsigned int Word209Supported : 1;
 unsigned int Reserved0 : 1;
};
struct __packed ata_nvcachecapabilities {
 unsigned int NVCachePowerModeEnabled : 1;
 unsigned int Reserved0 : 3;
 unsigned int NVCacheFeatureSetEnabled : 1;
 unsigned int Reserved1 : 3;
 unsigned int NVCachePowerModeVersion : 4;
 unsigned int NVCacheFeatureSetVersion : 4;
};
struct __packed ata_nvcacheoptions {
 __u8 NVCacheEstimatedTimeToSpinUpInSeconds;
 __u8 Reserved;
};


struct __packed ata_identifydata {
 struct ata_generalconfiguration GeneralConfiguration;
 __u16                           NumCylinders;
 __u16                           ReservedWord2;
 __u16                           NumHeads;
 __u16                           Retired1[2];
 __u16                           NumSectorsPerTrack;
 __u16                           VendorUnique1[3];
 __u8                            SerialNumber[20];
 __u16                           Retired2[2];
 __u16                           Obsolete1;
 __u8                            FirmwareRevision[8];
 __u8                            ModelNumber[40];
 __u8                            MaximumBlockTransfer;
 __u8                            VendorUnique2;
 __u16                           ReservedWord48;
 struct ata_capabilities         Capabilities;
 __u16                           ObsoleteWords51[2];
 unsigned int                    TranslationFieldsValid : 3;
 unsigned int                    Reserved3 : 13;
 __u16                           NumberOfCurrentCylinders;
 __u16                           NumberOfCurrentHeads;
 __u16                           CurrentSectorsPerTrack;
 __u32                           CurrentSectorCapacity;
 __u8                            CurrentMultiSectorSetting;
 unsigned int                    MultiSectorSettingValid : 1;
 unsigned int                    ReservedByte59 : 7;
 __u32                           UserAddressableSectors;
 __u16                           ObsoleteWord62;
 unsigned int                    MultiWordDMASupport : 8;
 unsigned int                    MultiWordDMAActive : 8;
 unsigned int                    AdvancedPIOModes : 8;
 unsigned int                    ReservedByte64 : 8;
 __u16                           MinimumMWXferCycleTime;
 __u16                           RecommendedMWXferCycleTime;
 __u16                           MinimumPIOCycleTime;
 __u16                           MinimumPIOCycleTimeIORDY;
 __u16                           ReservedWords69[6];
 unsigned int                    QueueDepth : 5;
 unsigned int                    ReservedWord75 : 11;
 __u16                           ReservedWords76[4];
 __u16                           MajorRevision;
 __u16                           MinorRevision;
 struct ata_commandset           CommandSetSupport;
 struct ata_commandset           CommandSetActive;
 unsigned int                    UltraDMASupport : 8;
 unsigned int                    UltraDMAActive : 8;
 __u16                           ReservedWord89[4];
 __u16                           HardwareResetResult;
 unsigned int                    CurrentAcousticValue : 8;
 unsigned int                    RecommendedAcousticValue : 8;
 __u16                           ReservedWord95[5];
 union{
 __u32                           Max48BitLBA[2];
 __u64                           UserAddressableSectors48;
 };
 __u16                           StreamingTransferTime;
 __u16                           ReservedWord105;
 struct ata_sectorsize           PhysicalLogicalSectorSize;
 __u16                           InterSeekDelay;
 __u16                           WorldWideName[4];
 __u16                           ReservedForWorldWideName128[4];
 __u16                           ReservedForTlcTechnicalReport;
 __u16                           WordsPerLogicalSector[2];
 struct ata_commandsetext        CommandSetSupportExt;
 struct ata_commandsetext        CommandSetActiveExt;
 __u16                           ReservedForExpandedSupportandActive[6];
 unsigned int                    MsnSupport : 2;
 unsigned int                    ReservedWord1274 : 14;
 struct ata_securitystatus       SecurityStatus;
 __u16                           ReservedWord129[31];
 struct ata_cfapowermodel        CfaPowerModel;
 __u16                           ReservedForCfaWord161[8];
 struct ata_datasetmanagementfeature DataSetManagementFeature;
 __u16                           ReservedForCfaWord170[6];
 __u16                           CurrentMediaSerialNumber[30];
 __u16                           ReservedWord206;
 __u16                           ReservedWord207[2];
 struct ata_blockalignment       BlockAlignment;
 __u16                           WriteReadVerifySectorCountMode3Only[2];
 __u16                           WriteReadVerifySectorCountMode2Only[2];
 struct ata_nvcachecapabilities  NVCacheCapabilities;
 __u16                           NVCacheSizeLSW;
 __u16                           NVCacheSizeMSW;
 __u16                           NominalMediaRotationRate;
 __u16                           ReservedWord218;
 struct ata_nvcacheoptions       NVCacheOptions;
 __u16                           ReservedWord220[35];
 unsigned int                    Signature : 8;
 unsigned int                    CheckSum : 8;
};

__COMPILER_PACK_POP

typedef struct __katadev katadev_t;
struct __katadev {
 struct ksdev            ad_sdev;  /*< Underlying storage device. */
 katabus_t               ad_bus;   /*< ATA Bus location. */
 katadrive_t             ad_drive; /*< ATA Drive location. */
 struct ata_identifydata ad_info;  /*< ATA device information. */
};

//////////////////////////////////////////////////////////////////////////
// Checks for a valid ATA device on the given bus and drive
// @return: KE_OK:     A valid device was detected
// @return: KE_NOSYS:  The given device isn't available
// @return: KE_DEVICE: The given device is errorous
extern __wunused __nonnull((1)) kerrno_t katadev_create(katadev_t *dev, katabus_t bus,
                                                        katadrive_t drive);

//////////////////////////////////////////////////////////////////////////
// Sleep the proper 400ns on the given bus
extern               void kata_sleep(katabus_t bus);
extern __wunused kerrno_t kata_poll(katabus_t bus, __u8 mask, __u8 state);

#define KATA_SECTORSIZE 512

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DEV_STORAGE_ATA_H__ */
