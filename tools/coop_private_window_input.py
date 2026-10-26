"""Window-local keyboard input for a process launched by private Windows QA.

Never focuses, shows or sends global input to a game. Callers must launch the
supplied executable in a new labels-production-qa* directory with a hidden
drawable. Keyboard mappings still pass through the runtime's normal SDL path.
"""
import ctypes as C
from ctypes import wintypes as W
import os
from pathlib import Path


class PrivateWindowInput:
    def __init__(self, process, executable):
        if os.name!='nt':raise RuntimeError('This private keyboard driver requires Windows')
        self.process=process;self.exe=Path(executable).resolve();self.down=set()
        assert self.exe.parent.name.startswith('labels-production-qa'),self.exe
        self.u=C.WinDLL('user32',use_last_error=True);k=C.WinDLL('kernel32',use_last_error=True)
        k.OpenProcess.argtypes=[W.DWORD,W.BOOL,W.DWORD];k.OpenProcess.restype=W.HANDLE
        k.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)]
        k.QueryFullProcessImageNameW.restype=W.BOOL;k.CloseHandle.argtypes=[W.HANDLE]
        handle=k.OpenProcess(0x1000,False,process.pid)
        if not handle:raise C.WinError(C.get_last_error())
        try:
            path=C.create_unicode_buffer(32768);length=W.DWORD(len(path))
            if not k.QueryFullProcessImageNameW(handle,0,path,C.byref(length)):raise C.WinError(C.get_last_error())
            assert Path(path.value).resolve()==self.exe,(path.value,self.exe)
        finally:k.CloseHandle(handle)
        self.u.GetWindowThreadProcessId.argtypes=[W.HWND,C.POINTER(W.DWORD)]
        self.u.GetWindowTextW.argtypes=[W.HWND,W.LPWSTR,C.c_int]
        self.u.IsWindowVisible.argtypes=[W.HWND];self.u.IsWindowVisible.restype=W.BOOL
        self.u.GetForegroundWindow.restype=W.HWND
        self.u.PostMessageW.argtypes=[W.HWND,W.UINT,W.WPARAM,W.LPARAM];self.u.PostMessageW.restype=W.BOOL
        self.u.MapVirtualKeyW.argtypes=[W.UINT,W.UINT];self.u.MapVirtualKeyW.restype=W.UINT
        self.enum_type=C.WINFUNCTYPE(W.BOOL,W.HWND,W.LPARAM)
        self.u.EnumWindows.argtypes=[self.enum_type,W.LPARAM]

    def window(self):
        assert self.process.poll() is None,'Private game exited'
        found=[]
        def visit(window,_):
            pid=W.DWORD();self.u.GetWindowThreadProcessId(window,C.byref(pid))
            if pid.value==self.process.pid:
                title=C.create_unicode_buffer(512);self.u.GetWindowTextW(window,title,len(title))
                if 'MegaManX4' in title.value or 'Mega Man X4' in title.value:found.append(window)
            return True
        self.u.EnumWindows(self.enum_type(visit),0)
        assert len(found)==1,found
        assert not self.u.IsWindowVisible(found[0]),'Private QA game became visible'
        return found[0]

    def foreground_pid(self):
        pid=W.DWORD();self.u.GetWindowThreadProcessId(self.u.GetForegroundWindow(),C.byref(pid))
        return pid.value

    def key(self,vk,down):
        window=self.window()
        assert self.foreground_pid()!=self.process.pid,'Private QA owns foreground focus'
        scan=self.u.MapVirtualKeyW(vk,0);parameter=1|(scan<<16)
        if vk in (37,38,39,40):parameter|=1<<24
        if not down:parameter|=3<<30
        assert self.u.PostMessageW(window,0x0007,0,0) # SDL-local focus event only.
        assert self.u.PostMessageW(window,0x0100 if down else 0x0101,vk,parameter)
        assert self.foreground_pid()!=self.process.pid,'Private QA captured foreground focus'
        if down:self.down.add(vk)
        else:self.down.discard(vk)

    def release_all(self):
        for vk in list(self.down):self.key(vk,False)
