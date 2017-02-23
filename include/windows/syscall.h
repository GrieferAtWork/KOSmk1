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
#ifndef __WINDOWS_SYSCALL_H__
#define __WINDOWS_SYSCALL_H__ 1

#include <kos/config.h>
#ifdef __KOS_HAVE_NTSYSCALL
#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN
// Table of system call can be found here:
// >> http://j00ru.vexillium.org/ntapi/
// Add support as is needed...

#define __NTSYSCALL_INTNO  0x2e /*< Windows uses 'int $0x2e' for system calls. */
#define SYS_NtCreateFile 	 0x0017

#ifndef __ASSEMBLY__
#endif /* !__ASSEMBLY__ */

// NtAcceptConnectPort 	 0x0000  	 0x0000  	 0x0000  	 0x0000  																										
// NtAccessCheck 	 0x0001  	 0x0001  	 0x0001  	 0x0001  																										
// NtAccessCheckAndAuditAlarm 	 0x0002  	 0x0002  	 0x0002  	 0x0002  																										
// NtAccessCheckByType 	    	    	    	    																										
// NtAccessCheckByTypeAndAuditAlarm 	    	    	    	    																										
// NtAccessCheckByTypeResultList 	    	    	    	    																										
// NtAccessCheckByTypeResultListAndAuditAlarm 	    	    	    	    																										
// NtAccessCheckByTypeResultListAndAuditAlarmByHandle 	    	    	    	    																										
// NtAcquireCMFViewOwnership 	    	    	    	    																										
// NtAddAtom 	 0x0003  	 0x0003  	 0x0003  	 0x0003  																										
// NtAddAtomEx 	    	    	    	    																										
// NtAddBootEntry 	    	    	    	    																										
// NtAddDriverEntry 	    	    	    	    																										
// NtAdjustGroupsToken 	 0x0004  	 0x0004  	 0x0004  	 0x0004  																										
// NtAdjustPrivilegesToken 	 0x0005  	 0x0005  	 0x0005  	 0x0005  																										
// NtAdjustTokenClaimsAndDeviceGroups 	    	    	    	    																										
// NtAlertResumeThread 	 0x0006  	 0x0006  	 0x0006  	 0x0006  																										
// NtAlertThread 	 0x0007  	 0x0007  	 0x0007  	 0x0007  																										
// NtAlertThreadByThreadId 	    	    	    	    																										
// NtAllocateLocallyUniqueId 	 0x0008  	 0x0008  	 0x0008  	 0x0008  																										
// NtAllocateReserveObject 	    	    	    	    																										
// NtAllocateUserPhysicalPages 	    	    	    	    																										
// NtAllocateUuids 	 0x0009  	 0x0009  	 0x0009  	 0x0009  																										
// NtAllocateVirtualMemory 	 0x000a  	 0x000a  	 0x000a  	 0x000a  																										
// NtAlpcAcceptConnectPort 	    	    	    	    																										
// NtAlpcCancelMessage 	    	    	    	    																										
// NtAlpcConnectPort 	    	    	    	    																										
// NtAlpcConnectPortEx 	    	    	    	    																										
// NtAlpcCreatePort 	    	    	    	    																										
// NtAlpcCreatePortSection 	    	    	    	    																										
// NtAlpcCreateResourceReserve 	    	    	    	    																										
// NtAlpcCreateSectionView 	    	    	    	    																										
// NtAlpcCreateSecurityContext 	    	    	    	    																										
// NtAlpcDeletePortSection 	    	    	    	    																										
// NtAlpcDeleteResourceReserve 	    	    	    	    																										
// NtAlpcDeleteSectionView 	    	    	    	    																										
// NtAlpcDeleteSecurityContext 	    	    	    	    																										
// NtAlpcDisconnectPort 	    	    	    	    																										
// NtAlpcImpersonateClientContainerOfPort 	    	    	    	    																										
// NtAlpcImpersonateClientOfPort 	    	    	    	    																										
// NtAlpcOpenSenderProcess 	    	    	    	    																										
// NtAlpcOpenSenderThread 	    	    	    	    																										
// NtAlpcQueryInformation 	    	    	    	    																										
// NtAlpcQueryInformationMessage 	    	    	    	    																										
// NtAlpcRevokeSecurityContext 	    	    	    	    																										
// NtAlpcSendWaitReceivePort 	    	    	    	    																										
// NtAlpcSetInformation 	    	    	    	    																										
// NtApphelpCacheControl 	    	    	    	    																										
// NtAreMappedFilesTheSame 	    	    	    	    																										
// NtAssignProcessToJobObject 	    	    	    	    																										
// NtAssociateWaitCompletionPacket 	    	    	    	    																										
// NtCallbackReturn 	 0x000b  	 0x000b  	 0x000b  	 0x000b  																										
// NtCancelDeviceWakeupRequest 	    	    	    	    																										
// NtCancelIoFile 	 0x000c  	 0x000c  	 0x000c  	 0x000c  																										
// NtCancelIoFileEx 	    	    	    	    																										
// NtCancelSynchronousIoFile 	    	    	    	    																										
// NtCancelTimer 	 0x000d  	 0x000d  	 0x000d  	 0x000d  																										
// NtCancelTimer2 	    	    	    	    																										
// NtCancelWaitCompletionPacket 	    	    	    	    																										
// NtClearAllSavepointsTransaction 	    	    	    	    																										
// NtClearEvent 	 0x000e  	 0x000e  	 0x000e  	 0x000e  																										
// NtClearSavepointTransaction 	    	    	    	    																										
// NtClose 	 0x000f  	 0x000f  	 0x000f  	 0x000f  																										
// NtCloseObjectAuditAlarm 	 0x0010  	 0x0010  	 0x0010  	 0x0010  																										
// NtCommitComplete 	    	    	    	    																										
// NtCommitEnlistment 	    	    	    	    																										
// NtCommitRegistryTransaction 	    	    	    	    																										
// NtCommitTransaction 	    	    	    	    																										
// NtCompactKeys 	    	    	    	    																										
// NtCompareObjects 	    	    	    	    																										
// NtCompareTokens 	    	    	    	    																										
// NtCompleteConnectPort 	 0x0011  	 0x0011  	 0x0011  	 0x0011  																										
// NtCompressKey 	    	    	    	    																										
// NtConnectPort 	 0x0012  	 0x0012  	 0x0012  	 0x0012  																										
// NtContinue 	 0x0013  	 0x0013  	 0x0013  	 0x0013  																										
// NtCreateChannel 	 0x00cd  	 0x00cc  	 0x00cc  	 0x00cc  																										
// NtCreateDebugObject 	    	    	    	    																										
// NtCreateDirectoryObject 	 0x0014  	 0x0014  	 0x0014  	 0x0014  																										
// NtCreateDirectoryObjectEx 	    	    	    	    																										
// NtCreateEnclave 	    	    	    	    																										
// NtCreateEnlistment 	    	    	    	    																										
// NtCreateEvent 	 0x0015  	 0x0015  	 0x0015  	 0x0015  																										
// NtCreateEventPair 	 0x0016  	 0x0016  	 0x0016  	 0x0016  																										
// NtCreateFile 	 0x0017  	 0x0017  	 0x0017  	 0x0017  																										
// NtCreateIRTimer 	    	    	    	    																										
// NtCreateIoCompletion 	 0x0018  	 0x0018  	 0x0018  	 0x0018  																										
// NtCreateJobObject 	    	    	    	    																										
// NtCreateJobSet 	    	    	    	    																										
// NtCreateKey 	 0x0019  	 0x0019  	 0x0019  	 0x0019  																										
// NtCreateKeyTransacted 	    	    	    	    																										
// NtCreateKeyedEvent 	    	    	    	    																										
// NtCreateLowBoxToken 	    	    	    	    																										
// NtCreateMailslotFile 	 0x001a  	 0x001a  	 0x001a  	 0x001a  																										
// NtCreateMutant 	 0x001b  	 0x001b  	 0x001b  	 0x001b  																										
// NtCreateNamedPipeFile 	 0x001c  	 0x001c  	 0x001c  	 0x001c  																										
// NtCreatePagingFile 	 0x001d  	 0x001e  	 0x001d  	 0x001d  																										
// NtCreatePartition 	    	    	    	    																										
// NtCreatePort 	 0x001e  	 0x001d  	 0x001e  	 0x001e  																										
// NtCreatePrivateNamespace 	    	    	    	    																										
// NtCreateProcess 	 0x001f  	 0x001f  	 0x001f  	 0x001f  																										
// NtCreateProcessEx 	    	    	    	    																										
// NtCreateProfile 	 0x0020  	 0x0020  	 0x0020  	 0x0020  																										
// NtCreateProfileEx 	    	    	    	    																										
// NtCreateRegistryTransaction 	    	    	    	    																										
// NtCreateResourceManager 	    	    	    	    																										
// NtCreateSection 	 0x0021  	 0x0021  	 0x0021  	 0x0021  																										
// NtCreateSemaphore 	 0x0022  	 0x0022  	 0x0022  	 0x0022  																										
// NtCreateSymbolicLinkObject 	 0x0023  	 0x0023  	 0x0023  	 0x0023  																										
// NtCreateThread 	 0x0024  	 0x0024  	 0x0024  	 0x0024  																										
// NtCreateThreadEx 	    	    	    	    																										
// NtCreateTimer 	 0x0025  	 0x0025  	 0x0025  	 0x0025  																										
// NtCreateTimer2 	    	    	    	    																										
// NtCreateToken 	 0x0026  	 0x0026  	 0x0026  	 0x0026  																										
// NtCreateTokenEx 	    	    	    	    																										
// NtCreateTransaction 	    	    	    	    																										
// NtCreateTransactionManager 	    	    	    	    																										
// NtCreateUserProcess 	    	    	    	    																										
// NtCreateWaitCompletionPacket 	    	    	    	    																										
// NtCreateWaitablePort 	    	    	    	    																										
// NtCreateWinStation 	    	 0x00d3  	    	    																										
// NtCreateWnfStateName 	    	    	    	    																										
// NtCreateWorkerFactory 	    	    	    	    																										
// NtDebugActiveProcess 	    	    	    	    																										
// NtDebugContinue 	    	    	    	    																										
// NtDelayExecution 	 0x0027  	 0x0027  	 0x0027  	 0x0027  																										
// NtDeleteAtom 	 0x0028  	 0x0028  	 0x0028  	 0x0028  																										
// NtDeleteBootEntry 	    	    	    	    																										
// NtDeleteDriverEntry 	    	    	    	    																										
// NtDeleteFile 	 0x0029  	 0x0029  	 0x0029  	 0x0029  																										
// NtDeleteKey 	 0x002a  	 0x002a  	 0x002a  	 0x002a  																										
// NtDeleteObjectAuditAlarm 	 0x002b  	 0x002b  	 0x002b  	 0x002b  																										
// NtDeletePrivateNamespace 	    	    	    	    																										
// NtDeleteValueKey 	 0x002c  	 0x002c  	 0x002c  	 0x002c  																										
// NtDeleteWnfStateData 	    	    	    	    																										
// NtDeleteWnfStateName 	    	    	    	    																										
// NtDeviceIoControlFile 	 0x002d  	 0x002d  	 0x002d  	 0x002d  																										
// NtDisableLastKnownGood 	    	    	    	    																										
// NtDisplayString 	 0x002e  	 0x002e  	 0x002e  	 0x002e  																										
// NtDrawText 	    	    	    	    																										
// NtDuplicateObject 	 0x002f  	 0x002f  	 0x002f  	 0x002f  																										
// NtDuplicateToken 	 0x0030  	 0x0030  	 0x0030  	 0x0030  																										
// NtEnableLastKnownGood 	    	    	    	    																										
// NtEnumerateBootEntries 	    	    	    	    																										
// NtEnumerateDriverEntries 	    	    	    	    																										
// NtEnumerateKey 	 0x0031  	 0x0031  	 0x0031  	 0x0031  																										
// NtEnumerateSystemEnvironmentValuesEx 	    	    	    	    																										
// NtEnumerateTransactionObject 	    	    	    	    																										
// NtEnumerateValueKey 	 0x0032  	 0x0032  	 0x0032  	 0x0032  																										
// NtExtendSection 	 0x0033  	 0x0033  	 0x0033  	 0x0033  																										
// NtFilterBootOption 	    	    	    	    																										
// NtFilterToken 	    	    	    	    																										
// NtFilterTokenEx 	    	    	    	    																										
// NtFindAtom 	 0x0034  	 0x0034  	 0x0034  	 0x0034  																										
// NtFlushBuffersFile 	 0x0035  	 0x0035  	 0x0035  	 0x0035  																										
// NtFlushBuffersFileEx 	    	    	    	    																										
// NtFlushInstallUILanguage 	    	    	    	    																										
// NtFlushInstructionCache 	 0x0036  	 0x0036  	 0x0036  	 0x0036  																										
// NtFlushKey 	 0x0037  	 0x0037  	 0x0037  	 0x0037  																										
// NtFlushProcessWriteBuffers 	    	    	    	    																										
// NtFlushVirtualMemory 	 0x0038  	 0x0038  	 0x0038  	 0x0038  																										
// NtFlushWriteBuffer 	 0x0039  	 0x0039  	 0x0039  	 0x0039  																										
// NtFreeUserPhysicalPages 	    	    	    	    																										
// NtFreeVirtualMemory 	 0x003a  	 0x003a  	 0x003a  	 0x003a  																										
// NtFreezeRegistry 	    	    	    	    																										
// NtFreezeTransactions 	    	    	    	    																										
// NtFsControlFile 	 0x003b  	 0x003b  	 0x003b  	 0x003b  																										
// NtGetCachedSigningLevel 	    	    	    	    																										
// NtGetCompleteWnfStateSubscription 	    	    	    	    																										
// NtGetContextThread 	 0x003c  	 0x003c  	 0x003c  	 0x003c  																										
// NtGetCurrentProcessorNumber 	    	    	    	    																										
// NtGetCurrentProcessorNumberEx 	    	    	    	    																										
// NtGetDevicePowerState 	    	    	    	    																										
// NtGetMUIRegistryInfo 	    	    	    	    																										
// NtGetNextProcess 	    	    	    	    																										
// NtGetNextThread 	    	    	    	    																										
// NtGetNlsSectionPtr 	    	    	    	    																										
// NtGetNotificationResourceManager 	    	    	    	    																										
// NtGetPlugPlayEvent 	 0x003d  	 0x003d  	 0x003d  	 0x003d  																										
// NtGetTickCount 	 0x003e  	 0x003e  	 0x003e  	 0x003e  																										
// NtGetWriteWatch 	    	    	    	    																										
// NtImpersonateAnonymousToken 	    	    	    	    																										
// NtImpersonateClientOfPort 	 0x003f  	 0x003f  	 0x003f  	 0x003f  																										
// NtImpersonateThread 	 0x0040  	 0x0040  	 0x0040  	 0x0040  																										
// NtInitializeEnclave 	    	    	    	    																										
// NtInitializeNlsFiles 	    	    	    	    																										
// NtInitializeRegistry 	 0x0041  	 0x0041  	 0x0041  	 0x0041  																										
// NtInitiatePowerAction 	    	    	    	    																										
// NtIsProcessInJob 	    	    	    	    																										
// NtIsSystemResumeAutomatic 	    	    	    	    																										
// NtIsUILanguageComitted 	    	    	    	    																										
// NtListTransactions 	    	    	    	    																										
// NtListenChannel 	 0x00ce  	 0x00cd  	 0x00cd  	 0x00cd  																										
// NtListenPort 	 0x0042  	 0x0042  	 0x0042  	 0x0042  																										
// NtLoadDriver 	 0x0043  	 0x0043  	 0x0043  	 0x0043  																										
// NtLoadEnclaveData 	    	    	    	    																										
// NtLoadKey 	 0x0044  	 0x0044  	 0x0044  	 0x0044  																										
// NtLoadKey2 	 0x0045  	 0x0045  	 0x0045  	 0x0045  																										
// NtLoadKeyEx 	    	    	    	    																										
// NtLockFile 	 0x0046  	 0x0046  	 0x0046  	 0x0046  																										
// NtLockProductActivationKeys 	    	    	    	    																										
// NtLockRegistryKey 	    	    	    	    																										
// NtLockVirtualMemory 	 0x0047  	 0x0047  	 0x0047  	 0x0047  																										
// NtMakePermanentObject 	    	    	    	    																										
// NtMakeTemporaryObject 	 0x0048  	 0x0048  	 0x0048  	 0x0048  																										
// NtManagePartition 	    	    	    	    																										
// NtMapCMFModule 	    	    	    	    																										
// NtMapUserPhysicalPages 	    	    	    	    																										
// NtMapUserPhysicalPagesScatter 	    	    	    	    																										
// NtMapViewOfSection 	 0x0049  	 0x0049  	 0x0049  	 0x0049  																										
// NtMarshallTransaction 	    	    	    	    																										
// NtModifyBootEntry 	    	    	    	    																										
// NtModifyDriverEntry 	    	    	    	    																										
// NtNotifyChangeDirectoryFile 	 0x004a  	 0x004a  	 0x004a  	 0x004a  																										
// NtNotifyChangeKey 	 0x004b  	 0x004b  	 0x004b  	 0x004b  																										
// NtNotifyChangeMultipleKeys 	    	    	    	    																										
// NtNotifyChangeSession 	    	    	    	    																										
// NtOpenChannel 	 0x00cf  	 0x00ce  	 0x00ce  	 0x00ce  																										
// NtOpenDirectoryObject 	 0x004c  	 0x004c  	 0x004c  	 0x004c  																										
// NtOpenEnlistment 	    	    	    	    																										
// NtOpenEvent 	 0x004d  	 0x004d  	 0x004d  	 0x004d  																										
// NtOpenEventPair 	 0x004e  	 0x004e  	 0x004e  	 0x004e  																										
// NtOpenFile 	 0x004f  	 0x004f  	 0x004f  	 0x004f  																										
// NtOpenIoCompletion 	 0x0050  	 0x0050  	 0x0050  	 0x0050  																										
// NtOpenJobObject 	    	    	    	    																										
// NtOpenKey 	 0x0051  	 0x0051  	 0x0051  	 0x0051  																										
// NtOpenKeyEx 	    	    	    	    																										
// NtOpenKeyTransacted 	    	    	    	    																										
// NtOpenKeyTransactedEx 	    	    	    	    																										
// NtOpenKeyedEvent 	    	    	    	    																										
// NtOpenMutant 	 0x0052  	 0x0052  	 0x0052  	 0x0052  																										
// NtOpenObjectAuditAlarm 	 0x0053  	 0x0053  	 0x0053  	 0x0053  																										
// NtOpenPartition 	    	    	    	    																										
// NtOpenPrivateNamespace 	    	    	    	    																										
// NtOpenProcess 	 0x0054  	 0x0054  	 0x0054  	 0x0054  																										
// NtOpenProcessToken 	 0x0055  	 0x0055  	 0x0055  	 0x0055  																										
// NtOpenProcessTokenEx 	    	    	    	    																										
// NtOpenRegistryTransaction 	    	    	    	    																										
// NtOpenResourceManager 	    	    	    	    																										
// NtOpenSection 	 0x0056  	 0x0056  	 0x0056  	 0x0056  																										
// NtOpenSemaphore 	 0x0057  	 0x0057  	 0x0057  	 0x0057  																										
// NtOpenSession 	    	    	    	    																										
// NtOpenSymbolicLinkObject 	 0x0058  	 0x0058  	 0x0058  	 0x0058  																										
// NtOpenThread 	 0x0059  	 0x0059  	 0x0059  	 0x0059  																										
// NtOpenThreadToken 	 0x005a  	 0x005a  	 0x005a  	 0x005a  																										
// NtOpenThreadTokenEx 	    	    	    	    																										
// NtOpenTimer 	 0x005b  	 0x005b  	 0x005b  	 0x005b  																										
// NtOpenTransaction 	    	    	    	    																										
// NtOpenTransactionManager 	    	    	    	    																										
// NtOpenWinStation 	    	 0x00d4  	    	    																										
// NtPlugPlayControl 	 0x005c  	 0x005c  	 0x005c  	 0x005c  																										
// NtPowerInformation 	    	    	    	    																										
// NtPrePrepareComplete 	    	    	    	    																										
// NtPrePrepareEnlistment 	    	    	    	    																										
// NtPrepareComplete 	    	    	    	    																										
// NtPrepareEnlistment 	    	    	    	    																										
// NtPrivilegeCheck 	 0x005d  	 0x005d  	 0x005d  	 0x005d  																										
// NtPrivilegeObjectAuditAlarm 	 0x005f  	 0x005f  	 0x005f  	 0x005f  																										
// NtPrivilegedServiceAuditAlarm 	 0x005e  	 0x005e  	 0x005e  	 0x005e  																										
// NtPropagationComplete 	    	    	    	    																										
// NtPropagationFailed 	    	    	    	    																										
// NtProtectVirtualMemory 	 0x0060  	 0x0060  	 0x0060  	 0x0060  																										
// NtPullTransaction 	    	    	    	    																										
// NtPulseEvent 	 0x0061  	 0x0061  	 0x0061  	 0x0061  																										
// NtQueryAttributesFile 	 0x0063  	 0x0063  	 0x0063  	 0x0063  																										
// NtQueryBootEntryOrder 	    	    	    	    																										
// NtQueryBootOptions 	    	    	    	    																										
// NtQueryDebugFilterState 	    	    	    	    																										
// NtQueryDefaultLocale 	 0x0064  	 0x0064  	 0x0064  	 0x0064  																										
// NtQueryDefaultUILanguage 	    	    	    	    																										
// NtQueryDirectoryFile 	 0x0065  	 0x0065  	 0x0065  	 0x0065  																										
// NtQueryDirectoryObject 	 0x0066  	 0x0066  	 0x0066  	 0x0066  																										
// NtQueryDriverEntryOrder 	    	    	    	    																										
// NtQueryEaFile 	 0x0067  	 0x0067  	 0x0067  	 0x0067  																										
// NtQueryEvent 	 0x0068  	 0x0068  	 0x0068  	 0x0068  																										
// NtQueryFullAttributesFile 	 0x0069  	 0x0069  	 0x0069  	 0x0069  																										
// NtQueryInformationAtom 	 0x0062  	 0x0062  	 0x0062  	 0x0062  																										
// NtQueryInformationEnlistment 	    	    	    	    																										
// NtQueryInformationFile 	 0x006a  	 0x006a  	 0x006a  	 0x006a  																										
// NtQueryInformationJobObject 	    	    	    	    																										
// NtQueryInformationPort 	 0x006c  	 0x006c  	 0x006c  	 0x006c  																										
// NtQueryInformationProcess 	 0x006d  	 0x006d  	 0x006d  	 0x006d  																										
// NtQueryInformationResourceManager 	    	    	    	    																										
// NtQueryInformationThread 	 0x006e  	 0x006e  	 0x006e  	 0x006e  																										
// NtQueryInformationToken 	 0x006f  	 0x006f  	 0x006f  	 0x006f  																										
// NtQueryInformationTransaction 	    	    	    	    																										
// NtQueryInformationTransactionManager 	    	    	    	    																										
// NtQueryInformationWorkerFactory 	    	    	    	    																										
// NtQueryInstallUILanguage 	    	    	    	    																										
// NtQueryIntervalProfile 	 0x0070  	 0x0070  	 0x0070  	 0x0070  																										
// NtQueryIoCompletion 	 0x006b  	 0x006b  	 0x006b  	 0x006b  																										
// NtQueryKey 	 0x0071  	 0x0071  	 0x0071  	 0x0071  																										
// NtQueryLicenseValue 	    	    	    	    																										
// NtQueryMultipleValueKey 	 0x0072  	 0x0072  	 0x0072  	 0x0072  																										
// NtQueryMutant 	 0x0073  	 0x0073  	 0x0073  	 0x0073  																										
// NtQueryObject 	 0x0074  	 0x0074  	 0x0074  	 0x0074  																										
// NtQueryOleDirectoryFile 	 0x0075  	 0x0075  	 0x0075  	 0x0075  																										
// NtQueryOpenSubKeys 	    	    	    	    																										
// NtQueryOpenSubKeysEx 	    	    	    	    																										
// NtQueryPerformanceCounter 	 0x0076  	 0x0076  	 0x0076  	 0x0076  																										
// NtQueryPortInformationProcess 	    	    	    	    																										
// NtQueryQuotaInformationFile 	    	    	    	    																										
// NtQuerySection 	 0x0077  	 0x0077  	 0x0077  	 0x0077  																										
// NtQuerySecurityAttributesToken 	    	    	    	    																										
// NtQuerySecurityObject 	 0x0078  	 0x0078  	 0x0078  	 0x0078  																										
// NtQuerySecurityPolicy 	    	    	    	    																										
// NtQuerySemaphore 	 0x0079  	 0x0079  	 0x0079  	 0x0079  																										
// NtQuerySymbolicLinkObject 	 0x007a  	 0x007a  	 0x007a  	 0x007a  																										
// NtQuerySystemEnvironmentValue 	 0x007b  	 0x007b  	 0x007b  	 0x007b  																										
// NtQuerySystemEnvironmentValueEx 	    	    	    	    																										
// NtQuerySystemInformation 	 0x007c  	 0x007c  	 0x007c  	 0x007c  																										
// NtQuerySystemInformationEx 	    	    	    	    																										
// NtQuerySystemTime 	 0x007d  	 0x007d  	 0x007d  	 0x007d  																										
// NtQueryTimer 	 0x007e  	 0x007e  	 0x007e  	 0x007e  																										
// NtQueryTimerResolution 	 0x007f  	 0x007f  	 0x007f  	 0x007f  																										
// NtQueryValueKey 	 0x0080  	 0x0080  	 0x0080  	 0x0080  																										
// NtQueryVirtualMemory 	 0x0081  	 0x0081  	 0x0081  	 0x0081  																										
// NtQueryVolumeInformationFile 	 0x0082  	 0x0082  	 0x0082  	 0x0082  																										
// NtQueryWinStationInformation 	    	 0x00d5  	    	    																										
// NtQueryWnfStateData 	    	    	    	    																										
// NtQueryWnfStateNameInformation 	    	    	    	    																										
// NtQueueApcThread 	 0x0083  	 0x0083  	 0x0083  	 0x0083  																										
// NtQueueApcThreadEx 	    	    	    	    																										
// NtRaiseException 	 0x0084  	 0x0084  	 0x0084  	 0x0084  																										
// NtRaiseHardError 	 0x0085  	 0x0085  	 0x0085  	 0x0085  																										
// NtReadFile 	 0x0086  	 0x0086  	 0x0086  	 0x0086  																										
// NtReadFileScatter 	 0x0087  	 0x0087  	 0x0087  	 0x0087  																										
// NtReadOnlyEnlistment 	    	    	    	    																										
// NtReadRequestData 	 0x0088  	 0x0088  	 0x0088  	 0x0088  																										
// NtReadVirtualMemory 	 0x0089  	 0x0089  	 0x0089  	 0x0089  																										
// NtRecoverEnlistment 	    	    	    	    																										
// NtRecoverResourceManager 	    	    	    	    																										
// NtRecoverTransactionManager 	    	    	    	    																										
// NtRegisterProtocolAddressInformation 	    	    	    	    																										
// NtRegisterThreadTerminatePort 	 0x008a  	 0x008a  	 0x008a  	 0x008a  																										
// NtReleaseCMFViewOwnership 	    	    	    	    																										
// NtReleaseKeyedEvent 	    	    	    	    																										
// NtReleaseMutant 	 0x008b  	 0x008b  	 0x008b  	 0x008b  																										
// NtReleaseSemaphore 	 0x008c  	 0x008c  	 0x008c  	 0x008c  																										
// NtReleaseWorkerFactoryWorker 	    	    	    	    																										
// NtRemoveIoCompletion 	 0x008d  	 0x008d  	 0x008d  	 0x008d  																										
// NtRemoveIoCompletionEx 	    	    	    	    																										
// NtRemoveProcessDebug 	    	    	    	    																										
// NtRenameKey 	    	    	    	    																										
// NtRenameTransactionManager 	    	    	    	    																										
// NtReplaceKey 	 0x008e  	 0x008e  	 0x008e  	 0x008e  																										
// NtReplacePartitionUnit 	    	    	    	    																										
// NtReplyPort 	 0x008f  	 0x008f  	 0x008f  	 0x008f  																										
// NtReplyWaitReceivePort 	 0x0090  	 0x0090  	 0x0090  	 0x0090  																										
// NtReplyWaitReceivePortEx 	    	    	    	    																										
// NtReplyWaitReplyPort 	 0x0091  	 0x0091  	 0x0091  	 0x0091  																										
// NtReplyWaitSendChannel 	 0x00d0  	 0x00cf  	 0x00cf  	 0x00cf  																										
// NtRequestDeviceWakeup 	    	    	    	    																										
// NtRequestPort 	 0x0092  	 0x0092  	 0x0092  	 0x0092  																										
// NtRequestWaitReplyPort 	 0x0093  	 0x0093  	 0x0093  	 0x0093  																										
// NtRequestWakeupLatency 	    	    	    	    																										
// NtResetEvent 	 0x0094  	 0x0094  	 0x0094  	 0x0094  																										
// NtResetWriteWatch 	    	    	    	    																										
// NtRestoreKey 	 0x0095  	 0x0095  	 0x0095  	 0x0095  																										
// NtResumeProcess 	    	    	    	    																										
// NtResumeThread 	 0x0096  	 0x0096  	 0x0096  	 0x0096  																										
// NtRevertContainerImpersonation 	    	    	    	    																										
// NtRollbackComplete 	    	    	    	    																										
// NtRollbackEnlistment 	    	    	    	    																										
// NtRollbackRegistryTransaction 	    	    	    	    																										
// NtRollbackSavepointTransaction 	    	    	    	    																										
// NtRollbackTransaction 	    	    	    	    																										
// NtRollforwardTransactionManager 	    	    	    	    																										
// NtSaveKey 	 0x0097  	 0x0097  	 0x0097  	 0x0097  																										
// NtSaveKeyEx 	    	    	    	    																										
// NtSaveMergedKeys 	    	    	    	    																										
// NtSavepointComplete 	    	    	    	    																										
// NtSavepointTransaction 	    	    	    	    																										
// NtSecureConnectPort 	    	    	    	    																										
// NtSendWaitReplyChannel 	 0x00d1  	 0x00d0  	 0x00d0  	 0x00d0  																										
// NtSerializeBoot 	    	    	    	    																										
// NtSetBootEntryOrder 	    	    	    	    																										
// NtSetBootOptions 	    	    	    	    																										
// NtSetCachedSigningLevel 	    	    	    	    																										
// NtSetCachedSigningLevel2 	    	    	    	    																										
// NtSetContextChannel 	 0x00d2  	 0x00d1  	 0x00d1  	 0x00d1  																										
// NtSetContextThread 	 0x0099  	 0x0099  	 0x0099  	 0x0099  																										
// NtSetDebugFilterState 	    	    	    	    																										
// NtSetDefaultHardErrorPort 	 0x009a  	 0x009a  	 0x009a  	 0x009a  																										
// NtSetDefaultLocale 	 0x009b  	 0x009b  	 0x009b  	 0x009b  																										
// NtSetDefaultUILanguage 	    	    	    	    																										
// NtSetDriverEntryOrder 	    	    	    	    																										
// NtSetEaFile 	 0x009c  	 0x009c  	 0x009c  	 0x009c  																										
// NtSetEvent 	 0x009d  	 0x009d  	 0x009d  	 0x009d  																										
// NtSetEventBoostPriority 	    	    	    	    																										
// NtSetHighEventPair 	 0x009e  	 0x009e  	 0x009e  	 0x009e  																										
// NtSetHighWaitLowEventPair 	 0x009f  	 0x009f  	 0x009f  	 0x009f  																										
// NtSetHighWaitLowThread 	 0x00a0  	 0x00a0  	 0x00a0  	 0x00a0  																										
// NtSetIRTimer 	    	    	    	    																										
// NtSetInformationDebugObject 	    	    	    	    																										
// NtSetInformationEnlistment 	    	    	    	    																										
// NtSetInformationFile 	 0x00a1  	 0x00a1  	 0x00a1  	 0x00a1  																										
// NtSetInformationJobObject 	    	    	    	    																										
// NtSetInformationKey 	 0x00a2  	 0x00a2  	 0x00a2  	 0x00a2  																										
// NtSetInformationObject 	 0x00a3  	 0x00a3  	 0x00a3  	 0x00a3  																										
// NtSetInformationProcess 	 0x00a4  	 0x00a4  	 0x00a4  	 0x00a4  																										
// NtSetInformationResourceManager 	    	    	    	    																										
// NtSetInformationSymbolicLink 	    	    	    	    																										
// NtSetInformationThread 	 0x00a5  	 0x00a5  	 0x00a5  	 0x00a5  																										
// NtSetInformationToken 	 0x00a6  	 0x00a6  	 0x00a6  	 0x00a6  																										
// NtSetInformationTransaction 	    	    	    	    																										
// NtSetInformationTransactionManager 	    	    	    	    																										
// NtSetInformationVirtualMemory 	    	    	    	    																										
// NtSetInformationWorkerFactory 	    	    	    	    																										
// NtSetIntervalProfile 	 0x00a7  	 0x00a7  	 0x00a7  	 0x00a7  																										
// NtSetIoCompletion 	 0x0098  	 0x0098  	 0x0098  	 0x0098  																										
// NtSetIoCompletionEx 	    	    	    	    																										
// NtSetLdtEntries 	 0x00a8  	 0x00a8  	 0x00a8  	 0x00a8  																										
// NtSetLowEventPair 	 0x00a9  	 0x00a9  	 0x00a9  	 0x00a9  																										
// NtSetLowWaitHighEventPair 	 0x00aa  	 0x00aa  	 0x00aa  	 0x00aa  																										
// NtSetLowWaitHighThread 	 0x00ab  	 0x00ab  	 0x00ab  	 0x00ab  																										
// NtSetQuotaInformationFile 	    	    	    	    																										
// NtSetSecurityObject 	 0x00ac  	 0x00ac  	 0x00ac  	 0x00ac  																										
// NtSetSystemEnvironmentValue 	 0x00ad  	 0x00ad  	 0x00ad  	 0x00ad  																										
// NtSetSystemEnvironmentValueEx 	    	    	    	    																										
// NtSetSystemInformation 	 0x00ae  	 0x00ae  	 0x00ae  	 0x00ae  																										
// NtSetSystemPowerState 	 0x00af  	 0x00af  	 0x00af  	 0x00af  																										
// NtSetSystemTime 	 0x00b0  	 0x00b0  	 0x00b0  	 0x00b0  																										
// NtSetThreadExecutionState 	    	    	    	    																										
// NtSetTimer 	 0x00b1  	 0x00b1  	 0x00b1  	 0x00b1  																										
// NtSetTimer2 	    	    	    	    																										
// NtSetTimerEx 	    	    	    	    																										
// NtSetTimerResolution 	 0x00b2  	 0x00b2  	 0x00b2  	 0x00b2  																										
// NtSetUuidSeed 	    	    	    	    																										
// NtSetValueKey 	 0x00b3  	 0x00b3  	 0x00b3  	 0x00b3  																										
// NtSetVolumeInformationFile 	 0x00b4  	 0x00b4  	 0x00b4  	 0x00b4  																										
// NtSetWinStationInformation 	    	 0x00d6  	    	    																										
// NtSetWnfProcessNotificationEvent 	    	    	    	    																										
// NtShutdownSystem 	 0x00b5  	 0x00b5  	 0x00b5  	 0x00b5  																										
// NtShutdownWorkerFactory 	    	    	    	    																										
// NtSignalAndWaitForSingleObject 	 0x00b6  	 0x00b6  	 0x00b6  	 0x00b6  																										
// NtSinglePhaseReject 	    	    	    	    																										
// NtStartProfile 	 0x00b7  	 0x00b7  	 0x00b7  	 0x00b7  																										
// NtStartTm 	    	    	    	    																										
// NtStopProfile 	 0x00b8  	 0x00b8  	 0x00b8  	 0x00b8  																										
// NtSubscribeWnfStateChange 	    	    	    	    																										
// NtSuspendProcess 	    	    	    	    																										
// NtSuspendThread 	 0x00b9  	 0x00b9  	 0x00b9  	 0x00b9  																										
// NtSystemDebugControl 	 0x00ba  	 0x00ba  	 0x00ba  	 0x00ba  																										
// NtTerminateJobObject 	    	    	    	    																										
// NtTerminateProcess 	 0x00bb  	 0x00bb  	 0x00bb  	 0x00bb  																										
// NtTerminateThread 	 0x00bc  	 0x00bc  	 0x00bc  	 0x00bc  																										
// NtTestAlert 	 0x00bd  	 0x00bd  	 0x00bd  	 0x00bd  																										
// NtThawRegistry 	    	    	    	    																										
// NtThawTransactions 	    	    	    	    																										
// NtTraceControl 	    	    	    	    																										
// NtTraceEvent 	    	    	    	    																										
// NtTranslateFilePath 	    	    	    	    																										
// NtUmsThreadYield 	    	    	    	    																										
// NtUnloadDriver 	 0x00be  	 0x00be  	 0x00be  	 0x00be  																										
// NtUnloadKey 	 0x00bf  	 0x00bf  	 0x00bf  	 0x00bf  																										
// NtUnloadKey2 	    	    	    	    																										
// NtUnloadKeyEx 	    	    	    	    																										
// NtUnlockFile 	 0x00c0  	 0x00c0  	 0x00c0  	 0x00c0  																										
// NtUnlockVirtualMemory 	 0x00c1  	 0x00c1  	 0x00c1  	 0x00c1  																										
// NtUnmapViewOfSection 	 0x00c2  	 0x00c2  	 0x00c2  	 0x00c2  																										
// NtUnmapViewOfSectionEx 	    	    	    	    																										
// NtUnsubscribeWnfStateChange 	    	    	    	    																										
// NtUpdateWnfStateData 	    	    	    	    																										
// NtVdmControl 	 0x00c3  	 0x00c3  	 0x00c3  	 0x00c3  																										
// NtW32Call 	 0x00cc  	    	    	    																										
// NtWaitForAlertByThreadId 	    	    	    	    																										
// NtWaitForDebugEvent 	    	    	    	    																										
// NtWaitForKeyedEvent 	    	    	    	    																										
// NtWaitForMultipleObjects 	 0x00c4  	 0x00c4  	 0x00c4  	 0x00c4  																										
// NtWaitForMultipleObjects32 	    	    	    	    																										
// NtWaitForSingleObject 	 0x00c5  	 0x00c5  	 0x00c5  	 0x00c5  																										
// NtWaitForWnfNotifications 	    	    	    	    																										
// NtWaitForWorkViaWorkerFactory 	    	    	    	    																										
// NtWaitHighEventPair 	 0x00c6  	 0x00c6  	 0x00c6  	 0x00c6  																										
// NtWaitLowEventPair 	 0x00c7  	 0x00c7  	 0x00c7  	 0x00c7  																										
// NtWorkerFactoryWorkerReady 	    	    	    	    																										
// NtWriteErrorLogEntry 	    	 0x00d7  	    	    																										
// NtWriteFile 	 0x00c8  	 0x00c8  	 0x00c8  	 0x00c8  																										
// NtWriteFileGather 	 0x00c9  	 0x00c9  	 0x00c9  	 0x00c9  																										
// NtWriteRequestData 	 0x00ca  	 0x00ca  	 0x00ca  	 0x00ca  																										
// NtWriteVirtualMemory 	 0x00cb  	 0x00cb  	 0x00cb  	 0x00cb  																										
// NtYieldExecution 	 0x00d3  	 0x00d2  	 0x00d2  	 0x00d2  	


__DECL_END
#endif /* __KOS_HAVE_NTSYSCALL */

#endif /* !__WINDOWS_SYSCALL_H__ */
