#!/usr/bin/env python
# coding: utf-8

import os, signal, socket, sys, unittest

sys.path.insert(0, '.')
import pinktrace
import pinktrace.socket
import pinktrace.syscall
import pinktrace.trace

TEST_UNIX_SOCKET = './TEST_UNIX_SOCKET'
class TestSocketLinux_01(unittest.TestCase):

    def test_01_decode_address_unix(self):
        pid = os.fork()
        if not pid: # child
            pinktrace.trace.me()
            os.kill(os.getpid(), signal.SIGSTOP)

            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.connect(TEST_UNIX_SOCKET)
        else: # parent
            pid, status = os.waitpid(pid, 0)
            self.assert_(os.WIFSTOPPED(status), "%#x" % status)
            self.assertEqual(os.WSTOPSIG(status), signal.SIGSTOP, "%#x" % status)

            # Loop until we get to the connect() system call as there's no
            # guarantee that other system calls won't be called beforehand.
            while True:
                pinktrace.trace.syscall_entry(pid, 0)
                pid, status = os.waitpid(pid, 0)
                self.assert_(os.WIFSTOPPED(status), "%#x" % status)
                self.assertEqual(os.WSTOPSIG(status), signal.SIGTRAP, "%#x" %  status)

                scno = pinktrace.syscall.get_no(pid)
                name = pinktrace.syscall.name(scno)
                if name == 'connect':
                    addr = pinktrace.socket.decode_address(pid, 1)
                    break

            self.assert_(isinstance(addr, pinktrace.socket.Address), "%r" % addr)
            self.assertEqual(addr.family, socket.AF_UNIX)
            self.assertEqual(addr.path, TEST_UNIX_SOCKET)

            try: pinktrace.trace.kill(pid)
            except OSError: pass

    def test_02_decode_address_inet(self):
        pid = os.fork()
        if not pid: # child
            pinktrace.trace.me()
            os.kill(os.getpid(), signal.SIGSTOP)

            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(("127.0.0.1", 12345))
        else: # parent
            pid, status = os.waitpid(pid, 0)
            self.assert_(os.WIFSTOPPED(status), "%#x" % status)
            self.assertEqual(os.WSTOPSIG(status), signal.SIGSTOP, "%#x" % status)

            # Loop until we get to the connect() system call as there's no
            # guarantee that other system calls won't be called beforehand.
            addr = None
            while True:
                pinktrace.trace.syscall_entry(pid, 0)
                pid, status = os.waitpid(pid, 0)
                self.assert_(os.WIFSTOPPED(status), "%#x" % status)
                self.assertEqual(os.WSTOPSIG(status), signal.SIGTRAP, "%#x" %  status)

                scno = pinktrace.syscall.get_no(pid)
                name = pinktrace.syscall.name(scno)
                if name == 'connect':
                    addr = pinktrace.socket.decode_address(pid, 1)
                    break

            self.assert_(isinstance(addr, pinktrace.socket.Address), "%r" % addr)
            self.assertEqual(addr.family, socket.AF_INET)
            self.assertEqual(addr.ip, "127.0.0.1")
            self.assertEqual(addr.port, 12345)

            try: pinktrace.trace.kill(pid)
            except OSError: pass

    def test_03_decode_address_fd_unix(self):
        pid = os.fork()
        if not pid: # child
            pinktrace.trace.me()
            os.kill(os.getpid(), signal.SIGSTOP)

            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.connect(TEST_UNIX_SOCKET)
        else: # parent
            pid, status = os.waitpid(pid, 0)
            self.assert_(os.WIFSTOPPED(status), "%#x" % status)
            self.assertEqual(os.WSTOPSIG(status), signal.SIGSTOP, "%#x" % status)

            # Loop until we get to the connect() system call as there's no
            # guarantee that other system calls won't be called beforehand.
            fd = -1
            addr = None
            while True:
                pinktrace.trace.syscall_entry(pid, 0)
                pid, status = os.waitpid(pid, 0)
                self.assert_(os.WIFSTOPPED(status), "%#x" % status)
                self.assertEqual(os.WSTOPSIG(status), signal.SIGTRAP, "%#x" %  status)

                scno = pinktrace.syscall.get_no(pid)
                name = pinktrace.syscall.name(scno)
                if name == 'connect':
                    addr, fd = pinktrace.socket.decode_address_fd(pid, 1)
                    break

            self.assert_(isinstance(addr, pinktrace.socket.Address), "%r" % addr)
            self.assert_(fd > 0, "%d" % fd)
            self.assertEqual(addr.family, socket.AF_UNIX)
            self.assertEqual(addr.path, TEST_UNIX_SOCKET)

            try: pinktrace.trace.kill(pid)
            except OSError: pass

    def test_04_decode_address_inet(self):
        pid = os.fork()
        if not pid: # child
            pinktrace.trace.me()
            os.kill(os.getpid(), signal.SIGSTOP)

            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(("127.0.0.1", 12345))
        else: # parent
            pid, status = os.waitpid(pid, 0)
            self.assert_(os.WIFSTOPPED(status), "%#x" % status)
            self.assertEqual(os.WSTOPSIG(status), signal.SIGSTOP, "%#x" % status)

            # Loop until we get to the connect() system call as there's no
            # guarantee that other system calls won't be called beforehand.
            fd = -1
            addr = None
            while True:
                pinktrace.trace.syscall_entry(pid, 0)
                pid, status = os.waitpid(pid, 0)
                self.assert_(os.WIFSTOPPED(status), "%#x" % status)
                self.assertEqual(os.WSTOPSIG(status), signal.SIGTRAP, "%#x" %  status)

                scno = pinktrace.syscall.get_no(pid)
                name = pinktrace.syscall.name(scno)
                if name == 'connect':
                    addr, fd = pinktrace.socket.decode_address_fd(pid, 1)
                    break

            self.assert_(isinstance(addr, pinktrace.socket.Address), "%r" % addr)
            self.assert_(fd > 0, "%d" % fd)
            self.assertEqual(addr.family, socket.AF_INET)
            self.assertEqual(addr.ip, "127.0.0.1")
            self.assertEqual(addr.port, 12345)

            try: pinktrace.trace.kill(pid)
            except OSError: pass

if __name__ == '__main__':
    unittest.main()
