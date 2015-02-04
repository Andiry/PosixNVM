// a module which counts calls to each function

#include "nv_common.h"
#include "perfcount.h"
#include <signal.h>

#define ENV_COUNT_FOP "NVP_COUNT_FOP"

BOOST_PP_SEQ_FOR_EACH(DECLARE_WITHOUT_ALIAS_FUNCTS_IWRAP, _perfcount_, ALLOPS_FINITEPARAMS_WPAREN)

unsigned int num_read;
unsigned int num_write;
unsigned int num_pread;
unsigned int num_pwrite;
unsigned long long size_read;
unsigned long long size_write;
unsigned long long size_pread;
unsigned long long size_pwrite;

RETT_OPEN _perfcount_OPEN(INTF_OPEN);
RETT_IOCTL _perfcount_IOCTL(INTF_IOCTL);
RETT_READ _perfcount_READ(INTF_READ);
RETT_WRITE _perfcount_WRITE(INTF_WRITE);
RETT_PREAD _perfcount_PREAD(INTF_PREAD);
RETT_PWRITE _perfcount_PWRITE(INTF_PWRITE);

MODULE_REGISTRATION_F("perfcount", _perfcount_)


#define COUNTVAR(FUNCT) stat_per_cpu FUNCT##_stat MY_ALIGNED;
#define COUNTVAR_IWRAP(r, data, elem) COUNTVAR(elem)

BOOST_PP_SEQ_FOR_EACH(COUNTVAR_IWRAP, x, ALLOPS_WPAREN)


#define COUNT_WRAP(FUNCT) \
	RETT_##FUNCT _perfcount_##FUNCT(INTF_##FUNCT) { \
		CHECK_RESOLVE_FILEOPS(_perfcount_); \
		RETT_##FUNCT ret; \
		timing_type start_time; \
		perf_start_timing(FUNCT##_stat, start_time); \
		ret = _perfcount_fileops->FUNCT(CALL_##FUNCT); \
		perf_end_timing(FUNCT##_stat, start_time); \
		return ret; \
	}
#define COUNT_WRAP_IWRAP(r, data, elem) COUNT_WRAP(elem)

BOOST_PP_SEQ_FOR_EACH(COUNT_WRAP_IWRAP, x, ALLOPS_PERFCOUNT_WPAREN)

RETT_OPEN _perfcount_OPEN(INTF_OPEN)
{
	CHECK_RESOLVE_FILEOPS(_perfcount_);

	RETT_OPEN ret;

	timing_type start_time;
	perf_start_timing(OPEN_stat, start_time);
	
	if (FLAGS_INCLUDE(oflag, O_CREAT))
	{
		va_list arg;
		va_start(arg, oflag);
		int mode = va_arg(arg, int);
		va_end(arg);
		ret = _perfcount_fileops->OPEN(path, oflag, mode);
	} else {
		ret = _perfcount_fileops->OPEN(path, oflag);
	}

	perf_end_timing(OPEN_stat, start_time);

	return ret;
}

RETT_IOCTL _perfcount_IOCTL(INTF_IOCTL)
{
	CHECK_RESOLVE_FILEOPS(_perfcount_);
	
	timing_type start_time;
	perf_start_timing(IOCTL_stat, start_time);
	
	va_list arg;
	va_start(arg, request);
	int* third = va_arg(arg, int*);

	RETT_IOCTL result = _perfcount_fileops->IOCTL(file, request, third);

	perf_end_timing(IOCTL_stat, start_time);
	
	return result;
}

RETT_READ _perfcount_READ(INTF_READ)
{
	RETT_READ ret;
	ret = _perfcount_fileops->READ(CALL_READ);

	num_read++;
	size_read += ret;
	return ret;
}

RETT_WRITE _perfcount_WRITE(INTF_WRITE)
{
	RETT_WRITE ret;
	ret = _perfcount_fileops->WRITE(CALL_WRITE);

	num_write++;
	size_write += ret;
	return ret;
}

RETT_PREAD _perfcount_PREAD(INTF_PREAD)
{
	RETT_PREAD ret;
	ret = _perfcount_fileops->PREAD(CALL_PREAD);

	num_pread++;
	size_pread += ret;
	return ret;
}

RETT_PWRITE _perfcount_PWRITE(INTF_PWRITE)
{
	RETT_PWRITE ret;
	ret = _perfcount_fileops->PWRITE(CALL_PWRITE);

	num_pwrite++;
	size_pwrite += ret;
	return ret;
}

void _perfcount_SIGUSR1_handler(int sig);

void _perfcount_init2(void) __attribute__((constructor));
void _perfcount_init2(void)
{
	#define COUNT_INIT(FUNCT) perf_clear_stat(FUNCT##_stat);
	#define COUNT_INIT_IWRAP(r, data, elem) COUNT_INIT(elem)
	BOOST_PP_SEQ_FOR_EACH(COUNT_INIT_IWRAP, x, ALLOPS_WPAREN)
	signal(SIGUSR1, _perfcount_SIGUSR1_handler);
}

void _perfcount_print_io_stats(void)
{
	MSG("==================== PERFCOUNT IO stats: ====================\n");
	MSG("READ: count %u, size %llu, average %llu\n", num_read,
		size_read, num_read ? size_read / num_read : 0);
	MSG("WRITE: count %u, size %llu, average %llu\n", num_write,
		size_write, num_write ? size_write / num_write : 0);
	MSG("PREAD: count %u, size %llu, average %llu\n", num_pread,
		size_pread, num_pread ? size_pread / num_pread : 0);
	MSG("PWRITE: count %u, size %llu, average %llu\n", num_pwrite,
		size_pwrite, num_pwrite ? size_pwrite / num_pwrite : 0);

	printf("==================== PERFCOUNT IO stats: ====================\n");
	printf("READ: count %u, size %llu, average %llu\n", num_read,
		size_read, num_read ? size_read / num_read : 0);
	printf("WRITE: count %u, size %llu, average %llu\n", num_write,
		size_write, num_write ? size_write / num_write : 0);
	printf("PREAD: count %u, size %llu, average %llu\n", num_pread,
		size_pread, num_pread ? size_pread / num_pread : 0);
	printf("PWRITE: count %u, size %llu, average %llu\n", num_pwrite,
		size_pwrite, num_pwrite ? size_pwrite / num_pwrite : 0);
}

void _perfcount_print(void) __attribute__((destructor));
void _perfcount_print(void)
{
	MSG("_perfcount_: Here are the function counts:\n");
	printf("==================== PERFCOUNT timing stats: ====================\n");
	#define COUNT_PRINT(FUNCT) perf_print_stat(NVP_PRINT_FD, FUNCT##_stat, #FUNCT);
	#define COUNT_PRINT_IWRAP(r, data, elem) COUNT_PRINT(elem)
	BOOST_PP_SEQ_FOR_EACH(COUNT_PRINT_IWRAP, x, ALLOPS_WPAREN)
	_perfcount_print_io_stats();
}

void _perfcount_SIGUSR1_handler(int sig)
{
	MSG("SIGUSR1: print stats\n");
	_perfcount_print();
}

