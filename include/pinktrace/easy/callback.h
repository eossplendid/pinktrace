/*
 * Copyright (c) 2010, 2012 Ali Polatel <alip@exherbo.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LpIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PINK_EASY_CALLBACK_H
#define _PINK_EASY_CALLBACK_H

/**
 * @file pinktrace/easy/callback.h
 * @brief Pink's easy ptrace(2) event callbacks
 * @defgroup pink_easy_callback Pink's easy ptrace(2) event callbacks
 * @ingroup pinktrace-easy
 * @{
 **/

#include <stdbool.h>
#include <pinktrace/easy/error.h>
#include <pinktrace/easy/process.h>

/**
 * Implies that the loop should be aborted immediately,
 * with error set to #PINK_EASY_ERROR_CALLBACK_ABORT.
 **/
#define PINK_EASY_CFLAG_ABORT		(1 << 0)

/**
 * Implies that the current process should be removed from the
 * process list. Useful for handling @e -ESRCH in callbacks.
 **/
#define PINK_EASY_CFLAG_DROP		(1 << 1)

/**
 * Implies that the signal won't be delivered to the tracee.
 * Only makes sense for "signal" callback.
 **/
#define PINK_EASY_CFLAG_SIGIGN		(1 << 2)

struct pink_easy_context;

/**
 * Errback for errors in the main process.
 * - Use pink_easy_context_get_error() to get the error code.
 *
 * @attention This errback @b must exist. Unless the user assigns a function,
 * the library uses a simple default implementation which is
 * pink_easy_errback_stderr()
 *
 * There are a few important points about this callback:
 * - The variable arguments give extra information about the error condition
 *   and they vary between different error conditions.
 * - After some error conditions, the global variable errno may also give
 *   information about the failure reason of the underlying library call.
 *
 * Here's a list of possible error conditions, in no particular order:
 *
 * @verbatim
     -----------------------------------------------------------------------------
     - Error             errno  Arguments                                        -
     -----------------------------------------------------------------------------
     - CALLBACK_ABORT    X      X (no errback, direct exit from loop)            -
     - ALLOC             +      const char *errctx                               -
     - ATTACH            +      pid_t pid                                        -
     - FORK              +      const char *errctx                               -
     - WAIT              +      -                                                -
     - TRACE             +      pink_easy_process_t *current, const char *errctx -
     - PROCESS           -      pink_easy_process_t *current, const char *errctx -
     -----------------------------------------------------------------------------
   @endverbatim
 *
 * @param ctx Tracing context
 * @param ... Variable arguments give extra information about the error.
 **/
typedef void (*pink_easy_errback_t) (const struct pink_easy_context *ctx, ...);

/**
 * Default errback which prints an informative message on standard error.
 *
 * @param ctx Tracing context
 **/
void pink_easy_errback_stderr(const struct pink_easy_context *ctx, ...);

/**
 * Errback for errors in the spawned child.
 *
 * @param e Error code
 * @return Child exists with this return value
 **/
typedef int (*pink_easy_errback_child_t) (pink_easy_child_error_t e);

/**
 * Default child errback which prints an informative message on standard error
 * and returns @e EXIT_FAILURE
 *
 * @param e Child error code
 **/
int pink_easy_errback_child_stderr(pink_easy_child_error_t e);

/**
 * Callback for process trace startup
 *
 * @param ctx Tracing context
 * @param current Attached process
 * @param parent Parent of the new process or NULL for initial processes
 **/
typedef void (*pink_easy_callback_startup_t) (const struct pink_easy_context *ctx,
		pink_easy_process_t *current, pink_easy_process_t *parent);

/**
 * Callback for process teardown
 *
 * This is the last callback which is called before the process is detached and
 * her entry is freed.
 *
 * @param ctx Tracing context
 * @param current Detached process
 **/
typedef void (*pink_easy_callback_teardown_t) (const struct pink_easy_context *ctx,
		const pink_easy_process_t *current);

/**
 * Callback for the end of tracing.
 *
 * This is called when the count of the process list drops to zero, or
 * @e waitpid(2) returns @e -ECHILD.
 *
 * @attention If this callback is NULL, pink_easy_loop() will just return with
 * success, which may not always be what you expect!
 *
 * @see pink_easy_loop()
 *
 * @param ctx Tracing context
 * @return This value is returned by pink_easy_loop()
 **/
typedef int (*pink_easy_callback_cleanup_t) (const struct pink_easy_context *ctx);

/**
 * Callback for system call traps
 *
 * @param ctx Tracing context
 * @param current Current child
 * @param entering true if the child is entering the system call, false otherwise
 * @return See PINK_EASY_CFLAG_* for flags to set in the return value.
 **/
typedef int (*pink_easy_callback_syscall_t) (const struct pink_easy_context *ctx,
		pink_easy_process_t *current, bool entering);

/**
 * Callback for successful @e execve(2)
 *
 * @note The bitness of the child is updated before this callback is called.
 *
 * @param ctx Tracing context
 * @param current Current child
 * @param old_bitness Old bitness of the child
 * @return See PINK_EASY_CFLAG_* for flags to set in the return value.
 **/
typedef int (*pink_easy_callback_exec_t) (const struct pink_easy_context *ctx,
		pink_easy_process_t *current, pink_bitness_t old_bitness);

/**
 * Callback for pre-exit notification
 *
 * @param ctx Tracing context
 * @param current Process ID
 * @param status Exit status
 * @return See PINK_EASY_CFLAG_* for flags to set in the return value.
 **/
typedef int (*pink_easy_callback_pre_exit_t) (const struct pink_easy_context *ctx,
		pink_easy_process_t *current, int status);

/**
 * Callback for stopping signal delivery
 *
 * @param ctx Tracing context
 * @param current Current process
 * @param status Stop status
 * @return See PINK_EASY_CFLAG_* for flags to set in the return value.
 **/
typedef int (*pink_easy_callback_signal_t) (const struct pink_easy_context *ctx,
		pink_easy_process_t *current, int status);

/**
 * Callback for genuine exit notification
 *
 * @param ctx Tracing context
 * @param pid Process ID of the child
 * @param status Exit status
 * @return See PINK_EASY_CFLAG_* for flags to set in the return value.
 **/
typedef int (*pink_easy_callback_exit_t) (const struct pink_easy_context *ctx,
		pid_t pid, int status);

/**
 * @brief Structure which represents a callback table
 **/
typedef struct pink_easy_callback_table {
	/** "error" errback **/
	pink_easy_errback_t error;
	/** "cerror" errback **/
	pink_easy_errback_child_t cerror;

	/** "startup" callback **/
	pink_easy_callback_startup_t startup;
	/** "teardown" callback **/
	pink_easy_callback_teardown_t teardown;
	/** "cleanup" callback **/
	pink_easy_callback_cleanup_t cleanup;

	/** "syscall" callback **/
	pink_easy_callback_syscall_t syscall;
	/** "exec" callback **/
	pink_easy_callback_exec_t exec;
	/** "pre_exit" callback **/
	pink_easy_callback_pre_exit_t pre_exit;
	/** "signal" callback **/
	pink_easy_callback_signal_t signal;
	/** "exit" callback **/
	pink_easy_callback_exit_t exit;
} pink_easy_callback_table_t;

PINK_END_DECL
/** @} */
#endif
