/*
 * Copyright (c) 2010, 2011, 2012 Ali Polatel <alip@exherbo.org>
 * Based in part upon strace which is:
 *   Copyright (c) 1991, 1992 Paul Kranenburg <pk@cs.few.eur.nl>
 *   Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
 *   Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
 *   Copyright (c) 1996-1999 Wichert Akkerman <wichert@cistron.nl>
 *   Copyright (c) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *                       Linux for s390 port by D.J. Barrow
 *                      <barrow_dj@mail.yahoo.com,djbarrow@de.ibm.com>
 *   Copyright (c) 2000 PocketPenguins Inc.  Linux for Hitachi SuperH
 *                      port by Greg Banks <gbanks@pocketpenguins.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <pinktrace/internal.h>
#include <pinktrace/pink.h>

bool pink_write_word_user(pid_t tid, long off, long val)
{
	return pink_ptrace(PTRACE_POKEUSER, tid, (void *)off, (void *)val) != -1;
}

bool pink_write_word_data(pid_t tid, long off, long val)
{
	return pink_ptrace(PTRACE_POKEDATA, tid, (void *)off, (void *)val) != -1;
}

static ssize_t _pink_process_vm_writev(pid_t tid,
		const struct iovec *local_iov,
		unsigned long liovcnt,
		const struct iovec *remote_iov,
		unsigned long riovcnt,
		unsigned long flags)
{
	ssize_t r;
#ifdef HAVE_PROCESS_VM_WRITEV
	r = process_vm_writev(tid,
			local_iov, liovcnt,
			remote_iov, riovcnt,
			flags);
#elif defined(__NR_process_vm_writev)
	r = syscall(__NR_process_vm_writev, (long)tid, local_iov, liovcnt, remote_iov, riovcnt, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
	return r;
}

static ssize_t _pink_write_vm_data_ptrace(pid_t tid, long addr, const char *src, size_t len)
{
	bool started;
	int n, m;
	union {
		long val;
		char x[sizeof(long)];
	} u;
	ssize_t count_written;

	started = false;
	count_written = 0;
	if (addr & (sizeof(long) - 1)) {
		/* addr not a multiple of sizeof(long) */
		n = addr - (addr & - sizeof(long)); /* residue */
		addr &= -sizeof(long); /* residue */
		m = MIN(sizeof(long) - n, len);
		memcpy(u.x, &src[n], m);
		if (!pink_write_word_data(tid, addr, u.val)) {
			/* Not started yet, thus we had a bogus address. */
			return -1;
		}
		started = true;
		addr += sizeof(long), src += m, len -= m, count_written += m;
	}
	while (len > 0) {
		m = MIN(sizeof(long), len);
		memcpy(u.x, src, m);
		if (!pink_write_word_data(tid, addr, u.val))
			return started ? count_written : -1;
		started = true;
		addr += sizeof(long), src += m, len -= m, count_written += m;
	}

	return count_written;
}

ssize_t pink_write_vm_data(pid_t tid, pink_abi_t abi, long addr,
		const char *src, size_t len)
{
#if PINK_ABIS_SUPPORTED > 1
	size_t wsize;

	if (!pink_abi_wordsize(abi, &wsize))
		return false;

	if (wsize < sizeof(addr))
		addr &= (1ul << 8 * wsize) - 1;
#endif

#if PINK_HAVE_PROCESS_VM_WRITEV
	struct iovec local[1], remote[1];
	local[0].iov_base = (void *)src;
	remote[0].iov_base = (void *)addr;
	local[0].iov_len = remote[0].iov_len = len;

	return _pink_process_vm_writev(tid,
			local, 1,
			remote, 1,
			/*flags:*/ 0
	);
#else
	return _pink_write_vm_data_ptrace(tid, addr, src, len);
#endif
}

bool pink_write_syscall(pid_t tid, pink_abi_t abi, long sysnum)
{
#if PINK_ARCH_ARM
# ifndef PTRACE_SET_SYSCALL
#  define PTRACE_SET_SYSCALL 23
# endif
	if (!pink_ptrace(PTRACE_SET_SYSCALL, tid, NULL, (void *)(long)(sysnum & 0xffff)))
		return false;
#elif PINK_ARCH_IA64
	if (abi == 1) { /* ia32 */
		if (!pink_write_word_user(tid, PT_R1, &sysnum))
			return false;
	} else {
		if (!pink_write_word_user(tid, PT_R15, sysnum))
			return false;
	}
#elif PINK_ARCH_POWERPC
	if (!pink_write_word_user(tid, sizeof(unsigned long)*PT_R0, sysnum))
		return false;
#elif PINK_ARCH_I386
	if (!pink_write_word_user(tid, 4 * ORIG_EAX, sysnum))
		return false;
#elif PINK_ARCH_X86_64 || PINK_ARCH_X32
	if (!pink_write_word_user(tid, 8 * ORIG_RAX, sysnum))
		return false;
#else
#error unsupported architecture
#endif
	return true;
}

bool pink_write_retval(pid_t tid, pink_abi_t abi, long retval, int error)
{
#if PINK_ARCH_ARM
	return pink_write_word_user(tid, 0, retval);
#elif PINK_ARCH_IA64
	long r8, r10;

	if (error) {
		r8 = -error;
		r10 = -1;
	} else {
		r8 = retval;
		r10 = 0;
	}

	return pink_write_word_user(tid, PT_R8, r8)
		&& pink_write_word_user(tid, PT_R10, r10);
#elif PINK_ARCH_POWERPC
#define SO_MASK 0x10000000
	long flags;

	if (!pink_read_word_user(tid, sizeof(unsigned long) * PT_CCR, &flags))
		return false;

	if (error) {
		retval = error;
		flags |= SO_MASK;
	} else {
		flags &= ~SO_MASK;
	}

	return pink_write_word_user(tid, sizeof(unsigned long) * PT_R3, retval) &&
		pink_write_word_user(tid, sizeof(unsigned long) * PT_CCR, flags);
#elif PINK_ARCH_I386
	if (error)
		retval = (long)-error;
	return pink_write_word_user(tid, 4 * EAX, retval);
#elif PINK_ARCH_X86_64 || PINK_ARCH_X32
	if (error)
		retval = (long)-error;
	return pink_write_word_user(tid, 8 * RAX, retval);
#else
#error unsupported architecture
#endif
}

bool pink_write_argument(pid_t tid, pink_abi_t abi, unsigned arg_index, long argval)
{
	if (arg_index >= PINK_MAX_ARGS) {
		errno = EINVAL;
		return false;
	}
#if PINK_ARCH_ARM
	if (arg_index < 5)
		return pink_write_word_user(tid, sizeof(long) * arg_index, argval);

	/* TODO: how to write arg_index=5? on ARM? */
	errno = ENOTSUP;
	return false;
#elif PINK_ARCH_IA64
	/* TODO: Implement pink_write_argument() on IA64 */
	errno = ENOTSUP;
	return false;
#elif PINK_ARCH_POWERPC
	return pink_write_word_user(tid, (arg_index == 0)
				? (sizeof(unsigned long) * PT_ORIG_R3)
				: ((arg_index + PT_R3) * sizeof(unsigned long)),
				argval);
#elif PINK_ARCH_I386
	switch (arg_index) {
	case 0: return pink_write_word_user(tid, 4 * EBX, argval);
	case 1: return pink_write_word_user(tid, 4 * ECX, argval);
	case 2: return pink_write_word_user(tid, 4 * EDX, argval);
	case 3: return pink_write_word_user(tid, 4 * ESI, argval);
	case 4: return pink_write_word_user(tid, 4 * EDI, argval);
	case 5: return pink_write_word_user(tid, 4 * EBP, argval);
	default: _pink_assert_not_reached();
	}
#elif PINK_ARCH_X86_64 || PINK_ARCH_X32
	switch (abi) {
	case 1: /* i386 ABI */
		switch (arg_index) {
		case 0: return pink_write_word_user(tid, 8 * RBX, argval);
		case 1: return pink_write_word_user(tid, 8 * RCX, argval);
		case 2: return pink_write_word_user(tid, 8 * RDX, argval);
		case 3: return pink_write_word_user(tid, 8 * RSI, argval);
		case 4: return pink_write_word_user(tid, 8 * RDI, argval);
		case 5: return pink_write_word_user(tid, 8 * RBP, argval);
		default: _pink_assert_not_reached();
		}
		break;
	case _PINK_ABI_X32: /* x86-64 or x32 ABI */
#if PINK_ARCH_X86_64
	case 0:
#endif
		switch (arg_index) {
		case 0: return pink_write_word_user(tid, 8 * RDI, argval);
		case 1: return pink_write_word_user(tid, 8 * RSI, argval);
		case 2: return pink_write_word_user(tid, 8 * RDX, argval);
		case 3: return pink_write_word_user(tid, 8 * R10, argval);
		case 4: return pink_write_word_user(tid, 8 * R8, argval);
		case 5: return pink_write_word_user(tid, 8 * R9, argval);
		default: _pink_assert_not_reached();
		}
		break;
	default:
		errno = EINVAL;
		return false;
	}
#else
#error unsupported architecture
#endif
}