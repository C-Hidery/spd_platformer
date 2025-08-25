#include <stdio.h>
#include "common.h"
//spd_dump second amendment By Ryan Crepa
//spd_dump By TomKing062
//SPDX-License-Identifier: GPL-3.0-or-later
//addon funcs by YC (SPRDClientCore-second-amendment)
const char* Version = "[1.1.2.0@_250726]";
int bListenLibusb = -1;
int gpt_failed = 1;
int m_bOpened = 0;
int fdl1_loaded = 0;
int fdl2_executed = 0;
int isKickMode = 0;
int isCMethod = 0;
int selected_ab = -1;
uint64_t fblk_size = 0;
const char* o_exception;
int init_stage = -1;
//device stage from SPRDClientCore
enum Stages {
	Nothing = -1,
	BROM = 0,
	FDL1 = 1,
	FDL2 = 2,
	SPRD3 = 3,
	SPRD4 = 4
} device_stage = Nothing, device_mode = Nothing;
//spd_dump protocol
char** str2;
int in_quote;
char* temp;
char str1[(ARGC_MAX - 1) * ARGV_LEN];
#define REOPEN_FREQ 2
void print_help() {
	//TODO
	DBG_LOG("spd_platformer usage:\n"
	"\nOptions\n"
	"\t--wait [TIME(second)]\n"
	"\t\tSpecifies the time to wait for the device to connect.\n"
	"\t--stage [NUMBER]|-r|--reconnect\n"
	"\t\tTry to reconnect device in BROM/FDL1/FDL2 stage. Any number behaves the same way.\n"
	"\t\t(A device in BROM/FDL1 stage can be reconnected infinite times, but may only once in fdl2 stage)\n"
	"\t--verbose [LEVEL]\n"
	"\t\tSets the verbosity level of the output (supports 0, 1, or 2).\n"
	"\t--kick\n"
	"\t\tConnects the device using the route boot_diag -> cali_diag -> dl_diag.\n"
	"\t--kickto [MODE]\n"
	"\t\tConnects the device using a custom route boot_diag -> custom_diag. Supported modes are 0-127.\n"
	"\t\t(mode 0 is `kickto 2` on ums9621, mode 1 = cali_diag, mode 2 = dl_diag; not all devices support mode 2).\n"
	"\t-?|-h|--help\n"
	"\t\tShow help and usage information\n"
	"\t--sync\n"
	"\t\tSync flashing setting.\n"
	"\t--usb-fd [CODE]\n"
	"\t\tConvert termux transfered usb port fd.(Android platform only!!!)\n"
	);
	DBG_LOG(
		"\nRuntime Commands\n"
		"\t->verbose [LEVEL]\n"
		"\t\tSets the verbosity level of the output (supports 0, 1, or 2).\n"
		"\t->timeout [TIME(ms)]\n"
		"\t\tSets the command timeout in milliseconds.\n"
		"\t->baudrate [RATE]\n\t\t(Windows SPRD driver only, and BROM/FDL2 stage only)\n"
		"\t\tSupported baudrates are 57600, 115200, 230400, 460800, 921600, 1000000, 2000000, 3250000, and 4000000.\n"
		"\t\t(While in u-boot/littlekernel source code, only 115200, 230400, 460800, and 921600 are listed.)\n"
		"\t->exec_addr [BINARY FILE] [ADDR] \n\t\t(BROM stage only)\n"
		"\t\tSends a binary file to the specified memory address to bypass the signature verification by BootRom for splloader/FDL1.\n"
		"\t\tUsed for CVE-2022-38694.\n"
		"\t->fdl [FILE PATH] [ADDR]\n"
		"\t\tSends a file (splloader, FDL1, FDL2, sml, trustos, teecfg) to the specified memory address.\n"
		"\t->exec\n"
		"\t\tExecutes a sent file in the FDL1 stage. Typically used with sml or FDL2 (also known as uboot/lk).\n"
		"\t->path [SAVE PATH]\n"
		"\t\tChanges the save directory for commands like r, read_part, read_spec, read_flash, and read_mem.\n"
		"\t->nand_id [ID]\n"
		"\t\tSpecifies the 4th NAND ID, affecting `read_part` or `read_spec` size calculation, default value is 0x15.\n"
		"\t->rawdata {0,1,2}\n\t\t(FDL2 stage only)\n"
		"\t\tRawdata protocol helps speed up `w` and `write_part(s)` commands, when rawdata > 0, `blk_size` will not effect write speed.\n"
		"\t\t(rawdata relays on u-boot/lk, so don't set it manually.\n"
		"\t->blk_size [VALUE(byte)]\n\t\t(FDL2 stage only)\n"
		"\t\tSets the block size, with a maximum of 65535 bytes. This option helps speed up `r`, `w`,`read_part`,`read_spec` and `write_part` commands.\n"
		"\t->r|read_part all|part_name|part_id\n"
		"\t\tWhen the partition table is available:\n"
		"\t\t\tr|read_part all: full backup (excludes blackbox, cache, userdata)\n"
		"\t\t\tr|read_part all_lite: full backup (excludes inactive slot partitions, blackbox, cache, and userdata)\n"
		"\t\t\tall/all_lite is not usable on NAND\n"
		"\t\tWhen the partition table is unavailable:\n"
		"\t\t\tr|read_part will auto-calculate part size (supports emmc/ufs and NAND).\n"
		"\t->read_spec part_name|part_id offset size [FILE]\n"
		"\t\tReads a specific partition to a file at the given offset and size.\n"
		"\t\t(You must give offset and read_size.)"
		"\t\t(read ubi on nand) read_spec system 0 ubi40m system.bin\n"
		"\t->read_parts [PARTITION TABLE XML]\n"
		"\t\tDump partitions from a list file (If the file name starts with \"ubi\", the size will be calculated using the NAND ID).\n"
		"\t->w|write_part part_name|part_id [FILE PATH]\n"
		"\t\tWrites the specified file to a partition.\n"
		"\t->write_parts|write_parts_a|write_parts_b save_location\n"
		"\t\tWrites all partitions dumped by read_parts.\n"
		"\t->wof part_name offset FILE\n"
		"\t\tWrites the specified file to a partition at the given offset.\n"
		"\t->wov part_name offset VALUE\n"
		"\t\tWrites the specified value (max is 0xFFFFFFFF) to a partition at the given offset.\n"
		"\t->e|erase_part part_name|part_id\n"
		"\t\tErases the specified partition.\n"
		"\t->erase_all\n"
		"\t\tErases all partitions. Use with caution!\n"
		"\t->part_table [FILE PATH]\n"
		"\t\tRead the partition table on emmc/ufs, not all FDL2 supports this command.\n"
		"\t->repartition [PARTITION TABLE XML]\n"
		"\t\tRepartitions based on partition list XML.\n"
		"\t->p|print\n"
		"\t\tPrints partition table\n"
		"\t->ps|part_size part_name\n"
		"\t\tDisplays the size of the specified partition.\n"
		"\t->cp|check_part part_name\n"
		"\t\tChecks if the specified partition exists.\n"
		"\t->verity {0,1}\n"
		"\t\tEnables or disables dm-verity on android 10(+).\n"
		"\t->set_active {a,b}\n"
		"\t\tSets the active slot on VAB devices.\n"
		"\t->firstmode [MODE ID]\n"
		"\t\tSets the mode the device will enter after reboot.\n"
		"Advanced commands:\n"
		"\t->skip_confirm {0,1}\n"
		"\t\tSkips all confirmation prompts(use with caution!)\n"
		"\t->keep_charge\n"
		"\t\tKeep charge in FDL1/FDL2 stage.\n"
		"\t->send_end_data {0,1}\n"
		"\t\tSends end data after file transfer.\n"
		"\t->rawdata {0,1,2}\n"
		"\t\tEnable raw_data mode for file sending to get better speed.\n"
		"\t\t(Not all FDL2 support.)\n"
		"\t->slot {0,1,2}\n"
		"\t\tSelect slot auto|a|b on VAB devices.\n"
		"\t->chip_uid\n"
		"\t\tReads the chip UID (FDL2 stage only).\n"
		"\t->transcode {0,1}\n"
		"\t\tEnable or disable transcode mode (FDL2 stage only).\n"
		"\t->add_part [PARTITION NAME]\n"
		"\t\tAdd a partition to the partition table (FDL2 stage only).\n"
		"\t\t(Only compatibility-method mode)"
		"\t->bootloader {0,1}\n"
		"\t\tSet the bootloader status (FDL2 stage only).\n"
		"Notice:\n"
		"\t1.The compatibility way to get part table sometimes can not get all partitions on your device\n"
		"\t2.Command `bootloader` : It is only supported on special FDL2 and requires trustos and sml partition files."
	);
	DBG_LOG(
		"\nExit Commands\n"
		"\t->reboot-recovery\n\t\tFDL2 only\n"
		"\t->reboot-fastboot\n\t\tFDL2 only\n"
		"\t->reset\n\t\tFDL2 and new FDL1\n"
		"\t->poweroff\n\t\tFDL2 and new FDL1\n"
	);
}
void ThrowExit() {
	if (init_stage = -1) {
		o_exception = "Can not init program functions";
	}
	else if (init_stage = 0) {
		o_exception = "Device connected with wrong mode";
	}
	else if (init_stage = 1) {
		o_exception = "Failed to handshake";
	}
	else if (init_stage = 2) {
		o_exception = "Failed to get device info";
	}
	
}

int main(int argc, char** argv) {
	ThrowExit;
	spdio_t* io = NULL;
	int ret, wait = 30 * REOPEN_FREQ;
	int keep_charge = 1, end_data = 0, blk_size = 0, skip_confirm = 1, highspeed = 0, cve_v2 = 0;
	int nand_info[3];
	int argcount = 0, stage = -1, nand_id = DEFAULT_NAND_ID;
	unsigned exec_addr = 0, baudrate = 0;
	int bootmode = -1, at = 0, async = 1;
	//Set up environment
#if !USE_LIBUSB
	extern DWORD curPort;
	DWORD* ports;
	//Channel9 init(Windows platform)
#else
	//libsub init(Linux/Android-termux)
	extern libusb_device* curPort;
	libusb_device** ports;
#endif
	char* execfile = malloc(ARGV_LEN);
	if (!execfile) { ThrowExit(); ERR_EXIT("%s: malloc failed\n",o_exception); }
	io = spdio_init(0);

#if USE_LIBUSB
#ifdef __ANDROID__
	int xfd = -1; // This store termux gived fd
	//libusb_device_handle *handle; // Use spdio_t.dev_handle
	//libusb_device* device; //use curPort
	struct libusb_device_descriptor desc;
	libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY);
#endif
	ret = libusb_init(NULL);
	if (ret < 0)
		ERR_EXIT("libusb_init failed: %s\n", libusb_error_name(ret));
#else
	io->handle = createClass();
	call_Initialize(io->handle);
#endif
	sprintf(fn_partlist, "partition_%lld.xml", (long long)time(NULL));
	printf("spd_platformer version 1.1.0.0\n");
	printf("Copyright (C) 2025 Ryan Crepa\n");
	printf("Core by TomKing062\n");
#if _DEBUG
	DBG_LOG("running:debug, core version:%s\n", Version);
#else
	DBG_LOG("running:stable, core version:%s\n", Version);
#endif
	int i = 1;
	while (argc > 1) {
		if (!strcmp(argv[1], "--wait")) {
			if (argc <= 2) { ThrowExit(); ERR_EXIT("%s: bad option\n",o_exception); }
			wait = atoi(argv[2]) * REOPEN_FREQ;
			argc -= 2; argv += 2;
		}
		else if (!strcmp(argv[1], "--verbose")) {
			if (argc <= 2) { ThrowExit(); ERR_EXIT("%s: bad option\n",o_exception); }
			io->verbose = atoi(argv[2]);
			argc -= 2; argv += 2;
		}
		else if (!strcmp(argv[1], "--stage")) {
			if (argc <= 2) { ThrowExit(); ERR_EXIT("%s: bad option\n",o_exception); }
			stage = 99;
			argc -= 2; argv += 2;
		}
		else if (strstr(argv[1], "-r")) {
			stage = 99;
			argc -= 1; argv += 1;
		}
		else if (strstr(argv[1], "help") || strstr(argv[1], "-h") || strstr(argv[1], "-?")) {
			print_help();
			return 0;
#ifdef __ANDROID__
		}
		else if (!strcmp(argv[1], "--usb-fd")) { // Termux spec
			if (argc <= 2) { ThrowExit(); ERR_EXIT("%s: bad option\n",o_exception); }
			xfd = atoi(argv[argc - 1]);
			argc -= 2; argv += 1;
#endif
		}
		else if (!strcmp(argv[1], "--kick")) {
			isKickMode = 1;
			at = 1;
			argc -= 1; argv += 1;
		}
		else if (!strcmp(argv[1], "--kickto")) {
			isKickMode = 1;
			if (argc <= 2) { ThrowExit(); ERR_EXIT("%s: bad option\n",o_exception); }
			bootmode = strtol(argv[2], NULL, 0); at = 0;
			argc -= 2; argv += 2;
		}
		else if (!strcmp(argv[1], "--sync")) {
			async = 0;
			argc -= 1; argv += 1;
		}
		else break;
	}
	if (stage == 99) { bootmode = -1; at = 0; }
#ifdef __ANDROID__
	bListenLibusb = 0;
	DEG_LOG(OP, "Try to convert termux transfered usb port fd.");
	// handle
	if (xfd < 0)
		ERR_EXIT("Example: termux-usb -e \"./spd_main --usb-fd\" /dev/bus/usb/xxx/xxx\n"
			"Provide --usb-fd if run on android\n");

	if (libusb_wrap_sys_device(NULL, (intptr_t)xfd, &io->dev_handle))
		ERR_EXIT("libusb_wrap_sys_device exit unconditionally!\n");

	curPort = libusb_get_device(io->dev_handle);
	if (libusb_get_device_descriptor(curPort, &desc))
		ERR_EXIT("libusb_get_device exit unconditionally!");

	DBG_LOG("Vendor ID: %04x\nProduct ID: %04x\n", desc.idVendor, desc.idProduct);
	if (desc.idVendor != 0x1782 || desc.idProduct != 0x4d00) {
		ERR_EXIT("It seems spec device not a spd device!\n");
	}
	call_Initialize_libusb(io);
#else
#if !USE_LIBUSB
	bListenLibusb = 0;
	if (at || bootmode >= 0) {
		io->hThread = CreateThread(NULL, 0, ThrdFunc, NULL, 0, &io->iThread);
		if (io->hThread == NULL) return -1;
		ChangeMode(io, wait / REOPEN_FREQ * 1000, bootmode, at);
		wait = 30 * REOPEN_FREQ;
		stage = -1;
	}
#else
	if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) { DBG_LOG("hotplug unsupported on this platform\n"); bListenLibusb = 0; bootmode = -1; at = 0; }
	if (at || bootmode >= 0) {
		startUsbEventHandle();
		ChangeMode(io, wait / REOPEN_FREQ * 1000, bootmode, at);
		wait = 30 * REOPEN_FREQ;
		stage = -1;
	}
	if (bListenLibusb < 0) startUsbEventHandle();
#endif
#if _WIN32
	if (!bListenLibusb) {
		if (io->hThread == NULL) io->hThread = CreateThread(NULL, 0, ThrdFunc, NULL, 0, &io->iThread);
		if (io->hThread == NULL) return -1;
	}
#if !USE_LIBUSB
	if (!m_bOpened && async) {
		if (FALSE == CreateRecvThread(io)) {
			io->m_dwRecvThreadID = 0;
			DEG_LOG(E,"Create Receive Thread Fail.");
		}
	}
#endif
#endif
	if (!m_bOpened) {
		DBG_LOG("<waiting for connection,mode:dl,%ds>\n", wait / REOPEN_FREQ);
		init_stage = 0;
		ThrowExit;
		for (i = 0; ; i++) {
#if USE_LIBUSB
			if (bListenLibusb) {
				if (curPort) {
					if (libusb_open(curPort, &io->dev_handle) >= 0) call_Initialize_libusb(io);
					else ERR_EXIT("Failed to connect\n");
					break;
				}
			}
			if (!(i % 4)) {
				if ((ports = FindPort(0x4d00))) {
					for (libusb_device** port = ports; *port != NULL; port++) {
						if (libusb_open(*port, &io->dev_handle) >= 0) {
							call_Initialize_libusb(io);
							curPort = *port;
							break;
						}
					}
					libusb_free_device_list(ports, 1);
					ports = NULL;
					if (m_bOpened) break;
				}
			}
			if (i >= wait)
				ERR_EXIT("libusb_open_device failed\n");
#else
			if (io->verbose) DBG_LOG("Cost: %.1f, Found: %d\n", (float)i / REOPEN_FREQ, curPort);
			if (curPort) {
				if (!call_ConnectChannel(io->handle, curPort, WM_RCV_CHANNEL_DATA, io->m_dwRecvThreadID)) ERR_EXIT("Connection failed\n");
				break;
			}
			if (!(i % 4)) {
				if ((ports = FindPort("SPRD U2S Diag"))) {
					for (DWORD* port = ports; *port != 0; port++) {
						if (call_ConnectChannel(io->handle, *port, WM_RCV_CHANNEL_DATA, io->m_dwRecvThreadID)) {
							curPort = *port;
							break;
						}
					}
					free(ports);
					ports = NULL;
					if (m_bOpened) break;
				}
			}
			if (i >= wait) {
				ThrowExit();
				ERR_EXIT("%s: Failed to find port.\n",o_exception);
		}
#endif
			usleep(1000000 / REOPEN_FREQ);
		}
	}
#endif
	io->flags |= FLAGS_TRANSCODE;
	if (stage != -1) {
		io->flags &= ~FLAGS_CRC16;
		encode_msg_nocpy(io, BSL_CMD_CONNECT, 0);
	}
	else encode_msg(io, BSL_CMD_CHECK_BAUD, NULL, 1);
	//handshake
	for (int i = 0; ; ++i) {
		//check if device is connected correctly.
		if (io->recv_buf[2] == BSL_REP_VER) {
			ret = BSL_REP_VER;
			memcpy(io->raw_buf + 4, io->recv_buf + 5, 5);
			io->raw_buf[2] = 0;
			io->raw_buf[3] = 5;
			io->recv_buf[2] = 0;
		}
		else if (io->recv_buf[2] == BSL_REP_VERIFY_ERROR ||
			io->recv_buf[2] == BSL_REP_UNSUPPORTED_COMMAND) {
			if (!fdl1_loaded) {
				ret = io->recv_buf[2];
				io->recv_buf[2] = 0;
			}
			else ERR_EXIT("Failed to connect to device: %s, please reboot your phone by pressing POWER and VOLUME_UP for 7-10 seconds.\n", o_exception);
		}
		else {
			//device correct, handshake operation
			send_msg(io);
			recv_msg(io);
			ret = recv_type(io);
		}
		//device can only recv BSL_REP_ACK or BSL_REP_VER or BSL_REP_VERIFY_ERROR
		init_stage = 1;
		ThrowExit();
		if (ret == BSL_REP_ACK || ret == BSL_REP_VER || ret == BSL_REP_VERIFY_ERROR) {
			//check stage
			if (ret == BSL_REP_VER) {
				if (fdl1_loaded == 1) {
					device_stage = FDL1;
					DEG_LOG(OP, "FDL1 connected."); 
					if (!memcmp(io->raw_buf + 4, "SPRD4", 5)) fdl2_executed = -1;
				}
				else {
					device_stage = BROM;
					DEG_LOG(OP, "Check baud BROM");
					if (!memcmp(io->raw_buf + 4, "SPRD4", 5)) { fdl1_loaded = -1; fdl2_executed = -1; }
				}
				DBG_LOG("[INFO] Device mode version: ");
				print_string(stdout, io->raw_buf + 4, READ16_BE(io->raw_buf + 2));
				
				encode_msg_nocpy(io, BSL_CMD_CONNECT, 0);
				if (send_and_check(io)) exit(1);
			}
			else if (ret == BSL_REP_VERIFY_ERROR) {
				encode_msg_nocpy(io, BSL_CMD_CONNECT, 0);
				if (fdl1_loaded != 1) {
					if (send_and_check(io)) exit(1);
				}
				else { i = -1; continue; }
			}
			if (fdl1_loaded == 1) {
				DEG_LOG(OP, "FDL1 connected."); 
				device_stage = FDL1;
				if (keep_charge) {
					encode_msg_nocpy(io, BSL_CMD_KEEP_CHARGE, 0);
					if (!send_and_check(io)) DEG_LOG(OP, "Keep charge FDL1.");
				}
			}
			else { DEG_LOG(OP, "BROM connected."); }
			device_stage = BROM;
			break;
		}
		//FDL2 response:UNSUPPORTED
		else if (ret == BSL_REP_UNSUPPORTED_COMMAND) {
			encode_msg_nocpy(io, BSL_CMD_DISABLE_TRANSCODE, 0);
			if (!send_and_check(io)) {
				io->flags &= ~FLAGS_TRANSCODE;
				DEG_LOG(OP, "Try to disable transcode 0x7D.");
			}
			fdl2_executed = 1;
			break;
		}
		//fail
		else if (i == 4) {
			if (stage != -1) { ERR_EXIT("Failed to connect: %s, please reboot your phone by pressing POWER and VOLUME_UP for 7-10 seconds.\n", o_exception); }
			else { encode_msg_nocpy(io, BSL_CMD_CONNECT, 0); stage++; i = -1; }
		}
		
	}
	if (isKickMode == 1) {
		device_mode = SPRD4;
	}
	else device_mode = SPRD3;
	char** save_argv = NULL;
	if (fdl1_loaded == -1) argc += 2;
	if (fdl2_executed == -1) argc += 1;
	init_stage = 2;
	ThrowExit;
	if (device_mode == SPRD3) {
		if (device_stage == BROM) {
			DEG_LOG(I, "Device stage: BROM/SPRD3");
		}
		else if (device_stage == FDL1) DEG_LOG(I, "Device stage: FDL1/SPRD3");
		else if (device_stage == FDL2) DEG_LOG(I, "Device stage: FDL2/SPRD3");
		else { ERR_EXIT("Failed to connect: %s, please reboot your phone by pressing POWER and VOLUME_UP for 7-10 seconds.\n", o_exception); }
	}
	else if (device_stage == SPRD4) {
		if (device_stage == BROM) {
			DEG_LOG(I, "Device stage: BROM/SPRD4");
		}
		else if (device_stage == FDL1) DEG_LOG(I, "Device stage: FDL1/SPRD4");
		else if (device_stage == FDL2) DEG_LOG(I, "Device stage: FDL2/SPRD4");
		else {  ERR_EXIT("Failed to connect: %s, please reboot your phone by pressing POWER and VOLUME_UP for 7-10 seconds.\n",o_exception); }
	}
	else { ThrowExit; ERR_EXIT("Failed to connect: %s, please reboot your phone by pressing POWER and VOLUME_UP for 7-10 seconds.\n", o_exception); }
	//get in interaction 
	
	while (1) {
		//TODO
		if (argc > 1) {
			str2 = (char**)malloc(argc * sizeof(char*));
			if (fdl1_loaded == -1) {
				save_argv = argv;
				str2[1] = "loadfdl";
				str2[2] = "0x0";
			}
			else if (fdl2_executed == -1) {
				if (!save_argv) save_argv = argv;
				str2[1] = "exec";
			}
			else {
				if (save_argv) { argv = save_argv; save_argv = NULL; }
				for (i = 1; i < argc; i++) str2[i] = argv[i];
			}
			argcount = argc;
			in_quote = -1;
		}
		else {
			char ifs = '"';
			str2 = (char**)malloc(ARGC_MAX * sizeof(char*));
			memset(str1, 0, sizeof(str1));
			argcount = 0;
			in_quote = 0;

			if (fdl2_executed > 0)
				DBG_LOG("[FDL2]: ");
			else if (fdl1_loaded > 0)
				DBG_LOG("[FDL1]: ");
			else
				DBG_LOG("[BROM]: ");
			ret = scanf("%[^\n]", str1);
			while ('\n' != getchar());

			temp = strtok(str1, " ");
			while (temp) {
				if (!in_quote) {
					argcount++;
					if (argcount == ARGC_MAX) break;
					str2[argcount] = (char*)malloc(ARGV_LEN);
					if (!str2[argcount]) ERR_EXIT("malloc failed\n");
					memset(str2[argcount], 0, ARGV_LEN);
				}
				if (temp[0] == '\'') ifs = '\'';
				if (temp[0] == ifs) {
					in_quote = 1;
					temp += 1;
				}
				else if (in_quote) {
					strcat(str2[argcount], " ");
				}

				if (temp[strlen(temp) - 1] == ifs) {
					in_quote = 0;
					temp[strlen(temp) - 1] = 0;
				}

				strcat(str2[argcount], temp);
				temp = strtok(NULL, " ");
			}
			argcount++;
		}
		if (argcount == 1) {
			str2[1] = malloc(1);
			if (str2[1]) str2[1][0] = '\0';
			else ERR_EXIT("malloc failed\n");
			argcount++;
		}
		//parse args and interacting command
		if (!strcmp(str2[1], "exec_addr")) {
			FILE* fi;
			if (0 == fdl1_loaded && argcount > 2) {
				exec_addr = strtoul(str2[3], NULL, 0);
				sprintf(execfile, str2[2]);
				fi = fopen(execfile, "r");
				if (fi == NULL) { DEG_LOG(W,"%s does not exist", execfile); exec_addr = 0; }
				else fclose(fi);
			}
			DEG_LOG(I,"Current CVE address is 0x%x", exec_addr);
			if (!strcmp(str2[1], "exec_addr2")) cve_v2 = 1;
			argc -= 3, argv += 3;
		}
		else if (!strcmp(str2[1], "send_file") || !strcmp(str2[1],"send_no_enddata")) {
			const char* fn; uint32_t addr = 0; FILE* fi;
			if (argcount <= 3) { DEG_LOG(W,"send/send_no_enddata FILE addr"); argc = 1; continue; }

			fn = str2[2];
			fi = fopen(fn, "r");
			if (fi == NULL) { DEG_LOG(E,"File does not exist."); argc -= 3; argv += 3; continue; }
			else fclose(fi);
			addr = strtoul(str2[3], NULL, 0);
			if (!strcmp(str2[1], "send_file")) send_file(io, fn, addr, end_data, 528, 0, 0);
			else send_file(io, fn, addr, 0, 528, 0, 0);
			argc -= 3; argv += 3;
		}
		else if (!strncmp(str2[1], "fdl",3)) {
			//remove loadfdl func and add exception detection
			const char* fn; uint32_t addr = 0; FILE* fi;
			int argchange = 3;
			fn = str2[2];
			if (argcount <= argchange) { DEG_LOG(W,"fdl FILE addr"); argc = 1; continue; }
			//convert to ulong
			addr = strtoul(str2[3], NULL, 0);
			//IS FDL2, NO NEED
			if (fdl2_executed > 0) {
				DEG_LOG(W,"FDL2 is already executed, skipped.");
				argc -= argchange; argv += argchange;
				continue;
			}
			//FDL2, NOT NEED TO SEND CVE FILE
			else if (fdl1_loaded > 0) {
				if (fdl2_executed != -1) {
					fi = fopen(fn, "r");
					if (fi == NULL) { DEG_LOG(W,"File does not exist."); argc -= argchange; argv += argchange; continue; }
					else fclose(fi);
					send_file(io, fn, addr, end_data, blk_size ? blk_size : 528, 0, 0);
				}
			}
			//FDL1, MAY NEED TO SEND CVE FILE
			else {
				if (fdl1_loaded != -1) {
					fi = fopen(fn, "r");
					if (fi == NULL) { DEG_LOG(W,"File does not exist.\n"); argc -= argchange; argv += argchange; continue; }
					else fclose(fi);
					if (cve_v2) {
						size_t execsize = send_file(io, fn, addr, 0, 528, 0, 0);
						int n, gapsize = exec_addr - addr - execsize;
						for (i = 0; i < gapsize; i += n) {
							n = gapsize - i;
							if (n > 528) n = 528;
							encode_msg_nocpy(io, BSL_CMD_MIDST_DATA, n);
							if (send_and_check(io)) exit(1);
						}
						fi = fopen(execfile, "rb");
						if (fi) {
							fseek(fi, 0, SEEK_END);
							n = ftell(fi);
							fseek(fi, 0, SEEK_SET);
							execsize = fread(io->temp_buf, 1, n, fi);
							fclose(fi);
						}
						encode_msg_nocpy(io, BSL_CMD_MIDST_DATA, execsize);
						if (send_and_check(io)) exit(1);
						free(execfile);
					}
					else {
						send_file(io, fn, addr, end_data, 528, 0, 0);
						if (exec_addr) {
							send_file(io, execfile, exec_addr, 0, 528, 0, 0);
							free(execfile);
						}
						else {
							encode_msg_nocpy(io, BSL_CMD_EXEC_DATA, 0);
							if (send_and_check(io)) exit(1);
						}
					}
				}
				else {
					encode_msg_nocpy(io, BSL_CMD_EXEC_DATA, 0);
					if (send_and_check(io)) exit(1);
				}
				DEG_LOG(OP,"Execute FDL1");
				if (addr == 0x5500 || addr == 0x65000800) {
					highspeed = 1;
					if (!baudrate) baudrate = 921600;
				}

				/* FDL1 (chk = sum) */
				io->flags &= ~FLAGS_CRC16;

				encode_msg(io, BSL_CMD_CHECK_BAUD, NULL, 1);
				for (i = 0; ; i++) {
					send_msg(io);
					recv_msg(io);
					if (recv_type(io) == BSL_REP_VER) break;
					DEG_LOG(W,"Failed to check baud, retry...");
					if (i == 4) { o_exception = "Failed to check baud FDL1."; ERR_EXIT("Can not execute FDL: %s,please reboot your phone by pressing POWER and VOL_UP for 7-10 seconds.\n",o_exception); }
					usleep(500000);
				}
				DEG_LOG(I,"Check baud FDL1 done.");

				DEG_LOG(I,"Device REP_version: ");
				print_string(stderr, io->raw_buf + 4, READ16_BE(io->raw_buf + 2));
				if (!memcmp(io->raw_buf + 4, "SPRD4", 5)) fdl2_executed = -1;
				//special FDL1 MEM, DISABLED FOR STABILITY
#if FDL1_DUMP_MEM
				//read dump mem
				int pagecount = 0;
				char* pdump;
				char chdump;
				FILE* fdump;
				fdump = my_fopen("memdump.bin", "wb");
				encode_msg(io, BSL_CMD_CHECK_BAUD, NULL, 1);
				while (1) {
					send_msg(io);
					ret = recv_msg(io);
					if (!ret) ERR_EXIT("timeout reached\n");
					if (recv_type(io) == BSL_CMD_READ_END) break;
					pdump = (char*)(io->raw_buf + 4);
					for (i = 0; i < 512; i++) {
						chdump = *(pdump++);
						if (chdump == 0x7d) {
							if (*pdump == 0x5d || *pdump == 0x5e) chdump = *(pdump++) + 0x20;
						}
						fputc(chdump, fdump);
					}
					DEG_LOG(I,"dump page count %d", ++pagecount);
				}
				fclose(fdump);
				DEG_LOG(I,"dump mem end");
				//end
#endif

				encode_msg_nocpy(io, BSL_CMD_CONNECT, 0);
				if (send_and_check(io)) exit(1);
				DEG_LOG(I,"FDL1 connected.");
#if !USE_LIBUSB
				if (baudrate) {
					uint8_t* data = io->temp_buf;
					WRITE32_BE(data, baudrate);
					encode_msg_nocpy(io, BSL_CMD_CHANGE_BAUD, 4);
					if (!send_and_check(io)) {
						DEG_LOG(OP,"Change baud FDL1 to %d", baudrate);
						call_SetProperty(io->handle, 0, 100, (LPCVOID)&baudrate);
					}
				}
#endif
				if (keep_charge) {
					encode_msg_nocpy(io, BSL_CMD_KEEP_CHARGE, 0);
					if (!send_and_check(io)) DEG_LOG(OP,"Keep charge FDL1.");
				}
				fdl1_loaded = 1;
			}
			argc -= argchange; argv += argchange;
		}
		else if (!strcmp(str2[1], "exec")) {
			//spd_dump exec command
			if (fdl2_executed > 0) {
				DEG_LOG(W, "FDL2 is already executed, skipped.");
				argc -= 1; argv += 1;
				continue;
			}
			else if (fdl1_loaded > 0) {
				memset(&Da_Info, 0, sizeof(Da_Info));
				encode_msg_nocpy(io, BSL_CMD_EXEC_DATA, 0);
				send_msg(io);
				// Feature phones respond immediately,
				// but it may take a second for a smartphone to respond.
				ret = recv_msg_timeout(io, 15000);
				if (!ret) { ThrowExit();  ERR_EXIT("%s: timeout reached\n", o_exception); }
				ret = recv_type(io);
				// Is it always bullshit?
				if (ret == BSL_REP_INCOMPATIBLE_PARTITION)
					get_Da_Info(io);
				else if (ret != BSL_REP_ACK) {
				ThrowExit();
				ERR_EXIT("%s: excepted response (0x%04x)\n",o_exception, ret);

			}
				DEG_LOG(OP,"Execute FDL2");
				encode_msg_nocpy(io, BSL_CMD_READ_FLASH_INFO, 0);
				send_msg(io);
				ret = recv_msg(io);
				if (ret) {
					ret = recv_type(io);
					if (ret != BSL_REP_READ_FLASH_INFO) DEG_LOG(E,"excepted response (0x%04x)\n", ret);
					else Da_Info.dwStorageType = 0x101;
					// need more samples to cover BSL_REP_READ_MCP_TYPE packet to nand_id/nand_info
					// for nand_id 0x15, packet is 00 9b 00 0c 00 00 00 00 00 02 00 00 00 00 08 00
				}
				if (Da_Info.bDisableHDLC) {
					encode_msg_nocpy(io, BSL_CMD_DISABLE_TRANSCODE, 0);
					if (!send_and_check(io)) {
						io->flags &= ~FLAGS_TRANSCODE;
						DEG_LOG(OP,"Try to disable transcode 0x7D.");
					}
				}
				if (Da_Info.bSupportRawData) {
					blk_size = 0xf800;
					io->ptable = partition_list(io, fn_partlist, &io->part_count);
					if (fdl2_executed) {
						Da_Info.bSupportRawData = 0;
						DEG_LOG(OP,"Raw data mode disabled for SPRD4.");
					}
					else {
						encode_msg_nocpy(io, BSL_CMD_ENABLE_RAW_DATA, 0);
						if (!send_and_check(io)) DEG_LOG(OP,"Raw data mode enabled.");
					}
				}
				

				else if (highspeed || Da_Info.dwStorageType == 0x103) {
					blk_size = 0xf800;
					io->ptable = partition_list(io, fn_partlist, &io->part_count);
				}
				else if (Da_Info.dwStorageType == 0x102) {
					io->ptable = partition_list(io, fn_partlist, &io->part_count);
				}
				else if (Da_Info.dwStorageType == 0x101) DEG_LOG(I,"Device storage is nand.");
				if (gpt_failed != 1) {
					if (selected_ab == 2) DEG_LOG(I,"Device is using slot b\n");
					else if (selected_ab == 1) DEG_LOG(I,"Device is using slot a\n");
					else {
						DEG_LOG(I,"Device is not using VAB\n");
						if (Da_Info.bSupportRawData) {
							DEG_LOG(I,"Raw data mode is supported (level is %u) ,but DISABLED for stability, you can set it manually.", (unsigned)Da_Info.bSupportRawData);
							Da_Info.bSupportRawData = 0;
						}
					}
				}
				else {
					io->Cptable = partition_list_d(io, fn_partlist);
					isCMethod = 1;
				}
				if (nand_id == DEFAULT_NAND_ID) {
					nand_info[0] = (uint8_t)pow(2, nand_id & 3); //page size
					nand_info[1] = 32 / (uint8_t)pow(2, (nand_id >> 2) & 3); //spare area size
					nand_info[2] = 64 * (uint8_t)pow(2, (nand_id >> 4) & 3); //block size
				}
				fdl2_executed = 1;
			}
			argc -= 1; argv += 1;
#if !USE_LIBUSB
		}
		else if (!strcmp(str2[1], "baudrate")) {
			if (argcount > 2) {
				baudrate = strtoul(str2[2], NULL, 0);
				if (fdl2_executed) call_SetProperty(io->handle, 0, 100, (LPCVOID)&baudrate);
			}
			DEG_LOG(I,"Baudrate is %u", baudrate);
			argc -= 2; argv += 2;
#endif
		}
		else if (!strcmp(str2[1], "path")) {
			//spd_dump func
			if (argcount > 2) strcpy(savepath, str2[2]);
			DEG_LOG(I,"Save dir is %s", savepath);
			argc -= 2; argv += 2;
		}
		else if (!strcmp(str2[1], "nand_id")) {
			//spd_dump func
			if (argcount > 2) {
				nand_id = strtol(str2[2], NULL, 0);
				nand_info[0] = (uint8_t)pow(2, nand_id & 3); //page size
				nand_info[1] = 32 / (uint8_t)pow(2, (nand_id >> 2) & 3); //spare area size
				nand_info[2] = 64 * (uint8_t)pow(2, (nand_id >> 4) & 3); //block size
			}
			DEG_LOG(I,"Current nand ID is 0x%x", nand_id);
			argc -= 2; argv += 2;
		}
		else if (!strcmp(str2[1], "read_flash")) {
			const char* fn; uint64_t addr, offset, size;
			if (argcount <= 5) { DEG_LOG(W,"read_flash addr offset size FILE"); argc = 1; continue; }

			addr = str_to_size(str2[2]);
			offset = str_to_size(str2[3]);
			size = str_to_size(str2[4]);
			fn = str2[5];
			if ((addr | size | offset | (addr + offset + size)) >> 32) { DEG_LOG(E,"32-bit limit reached"); argc -= 5; argv += 5; continue; }
			//func
			dump_flash(io, addr, offset, size, fn, blk_size ? blk_size : 1024);
			argc -= 5; argv += 5;

		}
		else if (!strcmp(str2[1], "bootloader")) {
			if (argcount < 2) { DEG_LOG(W, "bootloader {0,1}"); argc = 1; continue; }
			int status = strtol(str2[2], NULL, 0);
			if (status < 0 || status > 1) { DEG_LOG(W, "bootloader {0,1}"); argc -= 2; argv += 2; continue; }
			set_bootloader_status(io, status);
		}
		
		else if (!strcmp(str2[1], "erase_flash")) {
			uint64_t addr, size;
			if (argc <= 3) { DEG_LOG(W,"erase_flash addr size"); argc = 1; continue; }
			if (!skip_confirm)
				if (!check_confirm("erase flash")) {
					argc -= 3; argv += 3;
					continue;
				}
			addr = str_to_size(str2[2]);
			size = str_to_size(str2[3]);
			if ((addr | size | (addr + size)) >> 32)
				ERR_EXIT("Error: 32-bit limit reached\n");
			uint32_t* data = (uint32_t*)io->temp_buf;
			WRITE32_BE(data, addr);
			WRITE32_BE(data + 1, size);
			encode_msg_nocpy(io, BSL_CMD_ERASE_FLASH, 4 * 2);
			if (!send_and_check(io)) DEG_LOG(I,"Erase flash successfully: 0x%08x\n", (int)addr);
			argc -= 3; argv += 3;

		}
		else if (!strcmp(str2[1], "read_mem")) {
			const char* fn; uint64_t addr, size;
			if (argcount <= 4) { DEG_LOG(W,"read_mem addr size FILE"); argc = 1; continue; }

			addr = str_to_size(str2[2]);
			size = str_to_size(str2[3]);
			fn = str2[4];
			if ((addr | size | (addr + size)) >> 32) { DEG_LOG(E,"32-bit limit reached"); argc -= 4; argv += 4; continue; }
			dump_mem(io, addr, size, fn, blk_size ? blk_size : 1024);
			argc -= 4; argv += 4;

		}
		else if (!strcmp(str2[1], "part_size") || !strcmp(str2[1], "ps")) {
			const char* name;
			if (argcount <= 2) { DEG_LOG(W,"part_size|ps [partition name]"); argc = 1; continue; }

			name = str2[2];
			if (selected_ab < 0) select_ab(io);
			DEG_LOG(I, "%s: ", name);
			DEG_LOG(I,"%lld KB",(long long)check_partition(io, name, 1));
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "check_part") || !strcmp(str2[1],"cp")) {
			const char* name;
			if (argcount <= 2) { DEG_LOG(W,"check_part|cp [partition name]"); argc = 1; continue; }

			name = str2[2];
			if (selected_ab < 0) select_ab(io);
			long r = (long long)check_partition(io, name, 0);
			if (r == 1) {
				DEG_LOG(I, "%s: Exist.",name);
			}
			else if (r == 0) {
				DEG_LOG(I, "%s: NotExist.", name);
			}
			else DEG_LOG(E, "Failed to check.");
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "read_spec")) {
			const char* name, * fn; uint64_t offset, size;
			uint64_t realsize = 0;
			if (argcount <= 5) { DEG_LOG(W,"read_spec part_name offset size FILE\n(read ubi on nand) read_part system 0 ubi40m system.bin"); argc = 1; continue; }

			offset = str_to_size_ubi(str2[3], nand_info);
			size = str_to_size_ubi(str2[4], nand_info);
			fn = str2[5];

			name = str2[2];
			get_partition_info(io, name, 0);
			if (!gPartInfo.size) { DEG_LOG(W,"Partition not exist"); argc -= 5; argv += 5; continue; }

			if (0xffffffff == size) size = check_partition(io, gPartInfo.name, 1);
			if (offset + size < offset) { DEG_LOG(E,"64-bit limit reached"); argc -= 5; argv += 5; continue; }
			dump_partition(io, gPartInfo.name, offset, size, fn, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			argc -= 5; argv += 5;

			}
		else if (!strcmp(str2[1], "r") || !strcmp(str2[1],"read_part")) {
			const char* name = str2[2];
			int loop_count = 0, in_loop = 0;
			const char* list[] = { "vbmeta", "splloader", "uboot", "sml", "trustos", "teecfg", "boot", "recovery" };
			if (argcount <= 2) { DEG_LOG(W,"r|read_part all/all_lite/partition_name/part_id"); argc = 1; continue; }
			if (!strcmp(name, "preset_modem")) {
				if (gpt_failed == 1) io->ptable = partition_list(io, fn_partlist, &io->part_count);
				if (!io->part_count) { DEG_LOG(W,"Partition table not available"); argc -= 2; argv += 2; continue; }
				if (selected_ab > 0) { DEG_LOG(OP,"Saving slot info"); dump_partition(io, "misc", 0, 1048576, "misc.bin", blk_size); }
				for (i = 0; i < io->part_count; i++)
					if (0 == strncmp("l_", (*(io->ptable + i)).name, 2) || 0 == strncmp("nr_", (*(io->ptable + i)).name, 3)) {
						char dfile[40];
						snprintf(dfile, sizeof(dfile), "%s.bin", (*(io->ptable + i)).name);
						dump_partition(io, (*(io->ptable + i)).name, 0, (*(io->ptable + i)).size, dfile, blk_size ? blk_size : DEFAULT_BLK_SIZE);
					}
				argc -= 2; argv += 2;
				continue;
			}
			else if (!strcmp(name, "all")) {
				if (gpt_failed == 1) io->ptable = partition_list(io, fn_partlist, &io->part_count);
				if (!isCMethod) {
					if (!io->part_count) { DEG_LOG(W, "Partition table not available\n"); argc -= 2; argv += 2; continue; }
					dump_partition(io, "splloader", 0, 256 * 1024, "splloader.bin", blk_size ? blk_size : DEFAULT_BLK_SIZE);
					for (i = 0; i < io->part_count; i++) {
						char dfile[40];
						if (!strncmp((*(io->ptable + i)).name, "blackbox", 8)) continue;
						else if (!strncmp((*(io->ptable + i)).name, "cache", 5)) continue;
						else if (!strncmp((*(io->ptable + i)).name, "userdata", 8)) continue;
						snprintf(dfile, sizeof(dfile), "%s.bin", (*(io->ptable + i)).name);
						dump_partition(io, (*(io->ptable + i)).name, 0, (*(io->ptable + i)).size, dfile, blk_size ? blk_size : DEFAULT_BLK_SIZE);
					}
				}
				else {
					if(!io->part_count_c) { DEG_LOG(W, "Partition table not available\n"); argc -= 2; argv += 2; continue; }
					dump_partition(io, "splloader", 0, 256 * 1024, "splloader.bin", blk_size ? blk_size : DEFAULT_BLK_SIZE);
					for (i = 0; i < io->part_count_c; i++) {
						char dfile[40];
						if (!strncmp((*(io->Cptable + i)).name, "blackbox", 8)) continue;
						else if (!strncmp((*(io->Cptable + i)).name, "cache", 5)) continue;
						else if (!strncmp((*(io->Cptable + i)).name, "userdata", 8)) continue;
						snprintf(dfile, sizeof(dfile), "%s.bin", (*(io->Cptable + i)).name);
						dump_partition(io, (*(io->Cptable + i)).name, 0, (*(io->Cptable + i)).size, dfile, blk_size ? blk_size : DEFAULT_BLK_SIZE);
					}
				}
				argc -= 2; argv += 2;
				continue;
			}
			else if (!strcmp(name, "all_lite")) {
				if (gpt_failed == 1) io->ptable = partition_list(io, fn_partlist, &io->part_count);
				if (!isCMethod) {
					if (!io->part_count) { DEG_LOG(E, "Partition table not available\n"); argc -= 2; argv += 2; continue; }
					dump_partition(io, "splloader", 0, 256 * 1024, "splloader.bin", blk_size ? blk_size : DEFAULT_BLK_SIZE);
					for (i = 0; i < io->part_count; i++) {
						char dfile[40];
						size_t namelen = strlen((*(io->ptable + i)).name);
						if (!strncmp((*(io->ptable + i)).name, "blackbox", 8)) continue;
						else if (!strncmp((*(io->ptable + i)).name, "cache", 5)) continue;
						else if (!strncmp((*(io->ptable + i)).name, "userdata", 8)) continue;
						if (selected_ab == 1 && namelen > 2 && 0 == strcmp((*(io->ptable + i)).name + namelen - 2, "_b")) continue;
						else if (selected_ab == 2 && namelen > 2 && 0 == strcmp((*(io->ptable + i)).name + namelen - 2, "_a")) continue;
						snprintf(dfile, sizeof(dfile), "%s.bin", (*(io->ptable + i)).name);
						dump_partition(io, (*(io->ptable + i)).name, 0, (*(io->ptable + i)).size, dfile, blk_size ? blk_size : DEFAULT_BLK_SIZE);
					}
				}
				else {
					if(!io->part_count_c) { DEG_LOG(E, "Partition table not available\n"); argc -= 2; argv += 2; continue; }
					dump_partition(io, "splloader", 0, 256 * 1024, "splloader.bin", blk_size ? blk_size : DEFAULT_BLK_SIZE);
					for (i = 0; i < io->part_count_c; i++) {
						char dfile[40];
						size_t namelen = strlen((*(io->Cptable + i)).name);
						if (!strncmp((*(io->Cptable + i)).name, "blackbox", 8)) continue;
						else if (!strncmp((*(io->Cptable + i)).name, "cache", 5)) continue;
						else if (!strncmp((*(io->Cptable + i)).name, "userdata", 8)) continue;
						if (selected_ab == 1 && namelen > 2 && 0 == strcmp((*(io->Cptable + i)).name + namelen - 2, "_b")) continue;
						else if (selected_ab == 2 && namelen > 2 && 0 == strcmp((*(io->Cptable + i)).name + namelen - 2, "_a")) continue;
						snprintf(dfile, sizeof(dfile), "%s.bin", (*(io->Cptable + i)).name);
						dump_partition(io, (*(io->Cptable + i)).name, 0, (*(io->Cptable + i)).size, dfile, blk_size ? blk_size : DEFAULT_BLK_SIZE);
					}
				}
				argc -= 2; argv += 2;
				continue;
			}
			else {
				if (!strcmp(name, "preset_resign")) {
					loop_count = 7; name = list[loop_count]; in_loop = 1;
				}
			rloop:
				get_partition_info(io, name, 1);
				if (!gPartInfo.size) {
					if (loop_count) { name = list[--loop_count]; goto rloop; }
					DEG_LOG(E,"Partition not exist\n");
					argc -= 2; argv += 2;
					continue;
				}
			}
			char dfile[40];
			if (isdigit(str2[2][0])) snprintf(dfile, sizeof(dfile), "%s.img", gPartInfo.name);
			else if (in_loop) snprintf(dfile, sizeof(dfile), "%s.img", list[loop_count]);
			else snprintf(dfile, sizeof(dfile), "%s.img", name);
			dump_partition(io, gPartInfo.name, 0, gPartInfo.size, dfile, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			if (loop_count--) { name = list[loop_count]; goto rloop; }
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "read_parts")) {
			const char* fn; FILE* fi;
			if (argcount <= 2) { DEG_LOG(W,"read_parts partition_table_file"); argc = 1; continue; }
			fn = str2[2];
			fi = fopen(fn, "r");
			if (fi == NULL) { DEG_LOG(E,"File does not exist."); argc -= 2; argv += 2; continue; }
			else fclose(fi);
			dump_partitions(io, fn, nand_info, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			argc -= 2; argv += 2;

			}
		else if (!strcmp(str2[1], "part_table")) {
			if (!isCMethod) {
				if (argcount <= 2) { DEG_LOG(W, "part_table FILE"); argc = 1; continue; }
				if (gpt_failed == 1) io->ptable = partition_list(io, str2[2], &io->part_count);
				if (!io->part_count) { DEG_LOG(E, "Partition table not available"); argc -= 2; argv += 2; continue; }
				else {
					DBG_LOG("  0 %36s     256KB\n", "splloader");
					FILE* fo = my_fopen(str2[2], "wb");
					if (!fo) ERR_EXIT("Failed to open file\n");
					fprintf(fo, "<Partitions>\n");
					for (i = 0; i < io->part_count; i++) {
						DBG_LOG("%3d %36s %7lldMB\n", i + 1, (*(io->ptable + i)).name, ((*(io->ptable + i)).size >> 20));
						fprintf(fo, "    <Partition id=\"%s\" size=\"", (*(io->ptable + i)).name);
						if (i + 1 == io->part_count) fprintf(fo, "0x%x\"/>\n", ~0);
						else fprintf(fo, "%lld\"/>\n", ((*(io->ptable + i)).size >> 20));
					}
					fprintf(fo, "</Partitions>");
					fclose(fo);
					DEG_LOG(I, "Partition table saved to %s", str2[2]);
				}
			}
			else {
				if (argcount <= 2) { DEG_LOG(W, "part_table FILE"); argc = 1; continue; }
				int c = io->part_count_c;
				if (!c) { DEG_LOG(E, "Partition table not available"); argc -= 2; argv += 2; continue; }
				else {
					DBG_LOG("  0 %36s     256KB\n", "splloader");
					FILE* fo = my_fopen(str2[2], "wb");
					if (!fo) ERR_EXIT("Failed to open file\n");
					fprintf(fo, "<Partitions>\n");
					for (i = 0; i < c; i++) {
						DBG_LOG("%3d %36s %7lldMB\n", i + 1, (*(io->Cptable + i)).name, ((*(io->Cptable + i)).size >> 20));
						fprintf(fo, "    <Partition id=\"%s\" size=\"", (*(io->Cptable + i)).name);
						if (i + 1 == io->part_count_c) fprintf(fo, "0x%x\"/>\n", ~0);
						else fprintf(fo, "%lld\"/>\n", ((*(io->Cptable + i)).size >> 20));
					}
					if (check_partition(io, "userdata", 0)) {
						fprintf(fo, "    <Partition id=\"%s\" size=\"", "userdata");
						fprintf(fo, "0x%x\"/>\n", ~0);
					}
					fprintf(fo, "</Partitions>");
					fclose(fo);
					DEG_LOG(I, "Partition table saved to %s", str2[2]);
				}
			}
			
			
		
			
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "repartition")) {
			const char* fn; FILE* fi;
			if (argcount <= 2) { DEG_LOG(W,"repartition FILE"); argc = 1; continue; }
			fn = str2[2];
			fi = fopen(fn, "r");
			if (fi == NULL) { DEG_LOG(E,"File does not exist."); argc -= 2; argv += 2; continue; }
			else fclose(fi);
			if (skip_confirm) repartition(io, str2[2]);
			else if (check_confirm("repartition")) repartition(io, str2[2]);
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "erase_part") || !strcmp(str2[1], "e")) {
			const char* name = str2[2];
			if (argcount <= 2) { DEG_LOG(W,"erase_part part_name/part_id\n"); argc = 1; continue; }
			if (!strcmp(name, "all")) {
				if (!check_confirm("erase all")) {
					argc -= 2; argv += 2;
					continue;
				}
				strcpy(gPartInfo.name, "all");
			}
			else {
				if (!skip_confirm)
					if (!check_confirm("erase partition")) {
						argc -= 2; argv += 2;
						continue;
					}
				get_partition_info(io, name, 0);
			}
			if (!gPartInfo.size) { DBG_LOG("part not exist\n"); argc -= 2; argv += 2; continue; }
			erase_partition(io, gPartInfo.name);
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "erase_all")) {
			if (!check_confirm("erase all")) {
				argc -= 1; argv += 1;
				continue;
			}
			erase_partition(io, "all");
			argc -= 1; argv += 1;

		}
		else if (!strcmp(str2[1], "w") || !strcmp(str2[1],"flash")) {
			const char* fn; FILE* fi;
			const char* name = str2[2];
			if (argcount <= 3) { DEG_LOG(W,"w/flash part_name/part_id FILE\n"); argc = 1; continue; }
			fn = str2[3];
			fi = fopen(fn, "r");
			if (fi == NULL) { DEG_LOG(E,"File does not exist.\n"); argc -= 3; argv += 3; continue; }
			else fclose(fi);
			if (!skip_confirm)
				if (!check_confirm("flash partition")) {
					argc -= 3; argv += 3;
					continue;
				}
			get_partition_info(io, name, 0);
			if (!gPartInfo.size) { DEG_LOG(E,"Partition does not exist\n"); argc -= 3; argv += 3; continue; }

			load_partition_unify(io, gPartInfo.name, fn, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			argc -= 3; argv += 3;

		}
		else if (!strncmp(str2[1], "flash_parts", 11)) {
			if (argcount <= 2) { DEG_LOG(W,"flash_parts/flash_parts_a/flash_parts_b save_location\n"); argc = 1; continue; }
			int force_ab = 0;
			if (!strcmp(str2[1], "flash_parts_a")) force_ab = 1;
			else if (!strcmp(str2[1], "flash_parts_b")) force_ab = 2;
			if (skip_confirm || check_confirm("flash all partitions")) load_partitions(io, str2[2], blk_size ? blk_size : DEFAULT_BLK_SIZE, force_ab);
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "w_force")) {
			const char* fn; FILE* fi;
			const char* name = str2[2];
			if (argcount <= 3) { DEG_LOG(W,"w_force part_name/part_id FILE"); argc = 1; continue; }
			if (Da_Info.dwStorageType == 0x101) { DEG_LOG(E,"w_force is not allowed on NAND(UBI) devices"); argc -= 3; argv += 3; continue; }
			if (!io->part_count) { DEG_LOG(E,"Partition table not available\n"); argc -= 3; argv += 3; continue; }
			fn = str2[3];
			fi = fopen(fn, "r");
			if (fi == NULL) { DEG_LOG(E,"File does not exist."); argc -= 3; argv += 3; continue; }
			else fclose(fi);
			get_partition_info(io, name, 0);
			if (!gPartInfo.size) { DEG_LOG(E,"Partition does not exist"); argc -= 3; argv += 3; continue; }

			if (!strncmp(gPartInfo.name, "splloader", 9)) { DEG_LOG(E,"Partition 'splloader' can not be force-written!"); argc -= 3; argv += 3; continue; }
			else if (isdigit(str2[2][0])) load_partition_force(io, atoi(str2[2]) - 1, fn, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			else {
				for (i = 0; i < io->part_count; i++)
					if (!strcmp(gPartInfo.name, (*(io->ptable + i)).name)) {
						load_partition_force(io, i, fn, blk_size ? blk_size : DEFAULT_BLK_SIZE);
						break;
					}
			}
			argc -= 3; argv += 3;

		}
		else if (!strcmp(str2[1], "wof") || !strcmp(str2[1], "wov")) {
			uint64_t offset;
			size_t length;
			uint8_t* src;
			const char* name = str2[2];
			if (argcount <= 4) { DEG_LOG(W,"wof part_name offset FILE\nwov part_name offset VALUE(max is 0xFFFFFFFF)"); argc = 1; continue; }
			if (strstr(name, "fixnv") || strstr(name, "runtimenv") || strstr(name, "userdata")) {
				DEG_LOG(E,"Partitions 'fixnv_x', 'runtimenv', 'userdata' can not be written offsetly");
				argc -= 4; argv += 4;
				continue;
			}

			offset = str_to_size(str2[3]);
			if (!strcmp(str2[1], "wov")) {
				src = malloc(4);
				if (!src) { DEG_LOG(E,"malloc failed"); argc -= 4; argv += 4; continue; }
				length = 4;
				*(uint32_t*)src = strtoul(str2[4], NULL, 0);
			}
			else {
				const char* fn = str2[4];
				src = loadfile(fn, &length, 0);
				if (!src) { DEG_LOG(E,"Open %s failed", fn); argc -= 4; argv += 4; continue; }
			}
			w_mem_to_part_offset(io, name, offset, src, length, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			free(src);
			argc -= 4; argv += 4;

		}
		else if (!strcmp(str2[1], "blk_size") || !strcmp(str2[1], "bs")) {
			if (argcount <= 2) { DEG_LOG(E,"blk_size byte\n\tmax is 65535"); argc = 1; continue; }
			blk_size = strtol(str2[2], NULL, 0);
#ifndef _MYDEBUG
			blk_size = blk_size < 0 ? 0 :
				blk_size > 0xf800 ? 0xf800 : ((blk_size + 0x7FF) & ~0x7FF);
#else
			blk_size = blk_size < 0 ? 0 : blk_size;
#endif
			argc -= 2; argv += 2;

			}
		else if (!strcmp(str2[1], "fblk_size") || !strcmp(str2[1], "fbs")) {
				if (argcount <= 2) { DEG_LOG(E,"fblk_size [value]\n\tvalue unit: MB"); argc = 1; continue; }
				fblk_size = strtoull(str2[2], NULL, 0) * 1024 * 1024;
				argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "verity")) {
			if (argcount <= 2) { DEG_LOG(W,"verity {0,1}"); argc = 1; continue; }
			if (atoi(str2[2])) dm_enable(io, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			else {
				if (!io->part_count) {
					DEG_LOG(W,"Disable dm-verity needs a valid partition table or a write-verification-disabled FDL2");
					if (!skip_confirm)
						if (!check_confirm("disable dm-verity")) {
							argc -= 2; argv += 2;
							continue;
						}
				}
				dm_disable(io, blk_size ? blk_size : DEFAULT_BLK_SIZE);
			}
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "set_active")) {
			if (argcount <= 2) { DEG_LOG(W,"set_active {a,b}"); argc = 1; continue; }
			set_active(io, str2[2]);
			argc -= 2; argv += 2;

		}
		else if (!strcmp(str2[1], "add_part")) {
			if (isCMethod) {
				const char* name = str2[2];
				int n = io->part_count_c + 1;
				if (check_partition(io, name, 0)) {
					long long size = check_partition(io, name, 1);
					if (!size) { DEG_LOG(E, "Can not get partition size"); argc -= 2; argv += 2; continue; }
					else {
						add_partition(io, name, size);
					}
				}
				else {
					DEG_LOG(E, "Partition does not exist");
				}
			}
			else {
				DEG_LOG(E,"`add_part` command only supported in compatibility-method mode.");
			}
		}
		else if (!strcmp(str2[1], "p") || !strcmp(str2[1], "print")) {
			if (io->part_count) {
				DBG_LOG("  0 %36s     256KB\n", "splloader");
				for (i = 0; i < io->part_count; i++) {
					DBG_LOG("%3d %36s %7lldMB\n", i + 1, (*(io->ptable + i)).name, ((*(io->ptable + i)).size >> 20));
				}
			}
			else if (io->part_count_c) {
				DBG_LOG("  0 %36s     256KB\n", "splloader");
				for (i = 0; i < io->part_count_c; i++) {
					DBG_LOG("%3d %36s %7lldMB\n", i + 1, (*(io->Cptable + i)).name, ((*(io->Cptable + i)).size >> 20));
				}
				long long k = check_partition(io, "userdata", 1);
				DBG_LOG("%3d %36s %7lldMB\n", io->part_count_c + 1, "userdata", k / 1024 / 1024);
			}
			
			argc -= 1; argv += 1;

			}
		else if (!strcmp(str2[1], "firstmode")) {
				if (argcount <= 2) { DEG_LOG(W,"firstmode mode_id"); argc = 1; continue; }
				uint8_t* modebuf = malloc(4);
				if (!modebuf) ERR_EXIT("malloc failed\n");
				uint32_t mode = strtol(str2[2], NULL, 0) + 0x53464D00;
				memcpy(modebuf, &mode, 4);
				w_mem_to_part_offset(io, "miscdata", 0x2420, modebuf, 4, 0x1000);
				free(modebuf);
				argc -= 2; argv += 2;

				}
		else if (!strcmp(str2[1], "skip_confirm")) {
					if (argcount <= 2) { DEG_LOG(W,"skip_confirm {0,1}"); argc = 1; continue; }
					skip_confirm = atoi(str2[2]);
					argc -= 2; argv += 2;

					}
		else if (!strcmp(str2[1], "rawdata")) {
						if (argcount <= 2) { DEG_LOG(W,"rawdata {0,1,2}"); argc = 1; continue; }
						Da_Info.bSupportRawData = atoi(str2[2]);
						argc -= 2; argv += 2;

						}
		else if (!strcmp(str2[1], "slot")) {
							if (argcount <= 2) { DEG_LOG(W,"slot {0,1,2}"); argc = 1; continue; }
							selected_ab = atoi(str2[2]);
							argc -= 2; argv += 2;

							}
		else if (!strcmp(str2[1], "chip_uid")) {
								encode_msg_nocpy(io, BSL_CMD_READ_CHIP_UID, 0);
								send_msg(io);
								ret = recv_msg(io);
								if (!ret) ERR_EXIT("timeout reached\n");
								if ((ret = recv_type(io)) != BSL_REP_READ_CHIP_UID) {
									DEG_LOG(E,"excepted response (0x%04x)\n", ret); argc -= 1; argv += 1; continue;
								}

								DEG_LOG(I,"Response: chip_uid:");
								print_string(stderr, io->raw_buf + 4, READ16_BE(io->raw_buf + 2));
								argc -= 1; argv += 1;

								}
		else if (!strcmp(str2[1], "transcode")) {
			const char* se = str2[2];
			if (se == "1") {
				unsigned a, f;
				if (argcount <= 2) { DEG_LOG(W,"transcode {0,1}"); argc = 1; continue; }
				a = atoi(str2[2]);
				if (a >> 1) { DEG_LOG(E,"transcode {0,1}"); argc -= 2; argv += 2; continue; }
				f = (io->flags & ~FLAGS_TRANSCODE);
				io->flags = f | (a ? FLAGS_TRANSCODE : 0);
				
			}
			else if (se == "0") {
				unsigned a;
				if (argcount <= 2) { DEG_LOG(E,"transcode {0,1}"); argc = 1; continue; }
				a = atoi(str2[2]);
				if (a >> 1) { DEG_LOG(E, "transcode {0,1}"); argc -= 2; argv += 2; continue; }
				encode_msg_nocpy(io, BSL_CMD_DISABLE_TRANSCODE, 0);
				if (!send_and_check(io)) io->flags &= ~FLAGS_TRANSCODE;
				
			}
			argc -= 2; argv += 2;
		}
		
		else if (!strcmp(str2[1], "keep_charge")) {
											if (argcount <= 2) { DEG_LOG(W,"keep_charge {0,1}"); argc = 1; continue; }
											keep_charge = atoi(str2[2]);
											argc -= 2; argv += 2;

											}
		else if (!strcmp(str2[1], "timeout")) {
												if (argcount <= 2) { DEG_LOG(W,"timeout [time]\n\ttime unit: ms"); argc = 1; continue; }
												io->timeout = atoi(str2[2]);
												argc -= 2; argv += 2;

												}
		else if (!strcmp(str2[1], "send_end_data")) {
													if (argcount <= 2) { DEG_LOG(E,"send_end_data {0,1}"); argc = 1; continue; }
													end_data = atoi(str2[2]);
													argc -= 2; argv += 2;

													}
		else if (!strcmp(str2[1], "reset")) {
														if (!fdl1_loaded) {
															DEG_LOG(W, "FDL does not executed.");
															argc -= 1; argv += 1;
															continue;
														}
														encode_msg_nocpy(io, BSL_CMD_NORMAL_RESET, 0);
														if (!send_and_check(io)) break;

														}
		else if (!strcmp(str2[1], "reboot-recovery")) {
															if (!fdl1_loaded) {
																DEG_LOG(W, "FDL does not executed.");
																argc -= 1; argv += 1;
																continue;
															}
															char* miscbuf = malloc(0x800);
															if (!miscbuf) ERR_EXIT("malloc failed\n");
															memset(miscbuf, 0, 0x800);
															strcpy(miscbuf, "boot-recovery");
															w_mem_to_part_offset(io, "misc", 0, (uint8_t*)miscbuf, 0x800, 0x1000);
															free(miscbuf);
															encode_msg_nocpy(io, BSL_CMD_NORMAL_RESET, 0);
															if (!send_and_check(io)) break;

															}
		else if (!strcmp(str2[1], "reboot-fastboot")) {
																if (!fdl1_loaded) {
																	DEG_LOG(W, "FDL does not executed.");
																	argc -= 1; argv += 1;
																	continue;
																}
																char* miscbuf = malloc(0x800);
																if (!miscbuf) ERR_EXIT("malloc failed\n");
																memset(miscbuf, 0, 0x800);
																strcpy(miscbuf, "boot-recovery");
																strcpy(miscbuf + 0x40, "recovery\n--fastboot\n");
																w_mem_to_part_offset(io, "misc", 0, (uint8_t*)miscbuf, 0x800, 0x1000);
																free(miscbuf);
																encode_msg_nocpy(io, BSL_CMD_NORMAL_RESET, 0);
																if (!send_and_check(io)) break;

																}
		else if (!strcmp(str2[1], "poweroff")) {
																	if (!fdl1_loaded) {
																		DEG_LOG(W, "FDL does not executed.");
																		argc -= 1; argv += 1;
																		continue;
																	}
																	encode_msg_nocpy(io, BSL_CMD_POWER_OFF, 0);
																	if (!send_and_check(io)) break;

																	}
		else if (!strcmp(str2[1], "verbose")) {
																		if (argcount <= 2) { DEG_LOG(W,"verbose {0,1,2}"); argc = 1; continue; }
																		io->verbose = atoi(str2[2]);
																		argc -= 2; argv += 2;

																		}
		else if (strlen(str2[1])) {
			print_help();
			argc = 1;
		}
		if (in_quote != -1)
			for (i = 1; i < argcount; i++)
				free(str2[i]);
		free(str2);
		if (m_bOpened == -1) {
			DEG_LOG(E,"device unattached, exiting...");
			break;
		}
		
	}
	spdio_free(io);
	return 0;
}
