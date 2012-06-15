/*
 * Copyright (c) 2010, 2011, 2012 Ali Polatel <alip@exherbo.org>
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

#include <pinktrace/easy/internal.h>
#include <pinktrace/pink.h>
#include <pinktrace/easy/pink.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

bool pink_easy_call(pink_easy_context_t *ctx, pink_easy_child_func_t func, void *userdata)
{
	pid_t tid;
	pink_easy_process_t *current;

	tid = fork();
	if (tid < 0) {
		ctx->callback_table.error(ctx, PINK_EASY_ERROR_FORK, "fork");
		return false;
	} else if (tid == 0) { /* child */
		if (!pink_trace_me())
			_exit(ctx->callback_table.cerror(PINK_EASY_CHILD_ERROR_SETUP));
		kill(getpid(), SIGSTOP);
		_exit(func(userdata));
	}
	/* parent */
	PINK_EASY_INSERT_PROCESS(ctx, current);
	if (current == NULL) {
		pink_trace_kill(tid, -1, SIGKILL);
		return false;
	}
	current->tid = tid;
	current->tgid = -1;
	current->flags = PINK_EASY_PROCESS_STARTUP | PINK_EASY_PROCESS_IGNORE_ONE_SIGSTOP;
	return true;
}