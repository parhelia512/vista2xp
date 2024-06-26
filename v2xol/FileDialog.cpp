/* FileDialog.cpp --- v2xol.dll for vista2xp */
/* This file is public domain software.
   Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>. */
#include "targetverxp.h"
#include <initguid.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <cstdio>
#include <strsafe.h>
#include <new>
#include "FileDialog.hpp"
#include "../ShellItemArray.hpp"

#define DOGIF_ONLY_IF_ONE 0x8

extern const GUID IID_IUnknown_;
DEFINE_GUID(IID_IModalWindow_, 0xb4db1657, 0x70d7, 0x485e, 0x8e,0x3e, 0x6f,0xcb,0x5a,0x5c,0x18,0x02);
DEFINE_GUID(IID_IFileDialog_, 0x42f85136, 0xdb7e, 0x439c, 0x85,0xf1, 0xe4,0x07,0x5d,0x13,0x5f,0xc8);
DEFINE_GUID(IID_IFileDialogEvents_, 0x973510db, 0x7d7f, 0x452b, 0x89,0x75, 0x74,0xa8,0x58,0x28,0xd3,0x54);
DEFINE_GUID(IID_IFileOpenDialog_, 0xd57c7288, 0xd4ad, 0x4768, 0xbe,0x02, 0x9d,0x96,0x95,0x32,0xd9,0x60);
DEFINE_GUID(IID_IFileSaveDialog_, 0x84bccd23, 0x5fde, 0x4cdb, 0xae,0xa4, 0xaf,0x64,0xb8,0x3d,0x78,0xab);
DEFINE_GUID(CLSID_FileOpenDialog_, 0xdc1c5a9c, 0xe88a, 0x4dde, 0xa5,0xa1, 0x60,0xf8,0x2a,0x20,0xae,0xf7);
DEFINE_GUID(CLSID_FileSaveDialog_, 0xc0b4e2f3, 0xba21, 0x4773, 0x8d,0xba, 0x33,0x5e,0xc9,0x46,0xeb,0x8b);

static HRESULT SHGetIDListFromObjectForXP(IShellItem *psi, LPITEMIDLIST *ppidl)
{
    if (!ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;

    LPWSTR pszFilePath = NULL;
    HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    if (SUCCEEDED(hr))
    {
        *ppidl = ILCreateFromPathW(pszFilePath);
        if (!*ppidl)
            hr = E_OUTOFMEMORY;
        CoTaskMemFree(pszFilePath);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////

class MFileOpenDialog : public IFileOpenDialog
{
public:
    static MFileOpenDialog *CreateInstance();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IModalWindow interface
    STDMETHODIMP Show(HWND hwndOwner);

    // IFileDialog interface
    STDMETHODIMP SetFileTypes(
        UINT cFileTypes,
        const COMDLG_FILTERSPEC *rgFilterSpec);
    STDMETHODIMP SetFileTypeIndex(UINT iFileType);
    STDMETHODIMP GetFileTypeIndex(UINT *piFileType);
    STDMETHODIMP Advise(
        IFileDialogEvents *pfde,
        DWORD *pdwCookie);
    STDMETHODIMP Unadvise(DWORD dwCookie);
    STDMETHODIMP SetOptions(FILEOPENDIALOGOPTIONS fos);
    STDMETHODIMP GetOptions(FILEOPENDIALOGOPTIONS *pfos);
    STDMETHODIMP SetDefaultFolder(IShellItem *psi);
    STDMETHODIMP SetFolder(IShellItem *psi);
    STDMETHODIMP GetFolder(IShellItem **ppsi);
    STDMETHODIMP GetCurrentSelection(IShellItem **ppsi);
    STDMETHODIMP SetFileName(LPCWSTR pszName);
    STDMETHODIMP GetFileName(LPWSTR *pszName);
    STDMETHODIMP SetTitle(LPCWSTR pszTitle);
    STDMETHODIMP SetOkButtonLabel(LPCWSTR pszText);
    STDMETHODIMP SetFileNameLabel(LPCWSTR pszLabel);
    STDMETHODIMP GetResult(IShellItem **ppsi);
    STDMETHODIMP AddPlace(IShellItem *psi, FDAP fdap);
    STDMETHODIMP SetDefaultExtension(LPCWSTR pszDefaultExtension);
    STDMETHODIMP Close(HRESULT hr);
    STDMETHODIMP SetClientGuid(REFGUID guid);
    STDMETHODIMP ClearClientData();
    STDMETHODIMP SetFilter(IShellItemFilter *pFilter);

    // IFileOpenDialog interface
    STDMETHODIMP GetResults(IShellItemArray **ppenum);
    STDMETHODIMP GetSelectedItems(IShellItemArray **ppsai);

protected:
    LONG m_nRefCount;
    HWND m_hwnd;
    LPITEMIDLIST m_pidlSelected;
    LPITEMIDLIST m_pidlDefFolder;
    BOOL m_bDoSave;
    FILEOPENDIALOGOPTIONS m_options;
    LPWSTR m_pszzFiles;
    LPWSTR m_pszTitle;
    LPWSTR m_pszzFilter;
    IFileDialogEvents *m_pEvents;
    DWORD m_dwCookie;
    WCHAR m_szFile[MAX_PATH];
    WCHAR m_szDefExt[32];
    OPENFILENAMEW m_ofn;
    BROWSEINFO m_bi;

    MFileOpenDialog();
    virtual ~MFileOpenDialog();
    void Init();

    IFileDialog *GetFD();
    BOOL UpdateFlags();
    BOOL IsFolderDialog() const;
    LPITEMIDLIST GetFolderIDList();
    BOOL VerifyFileType();

    static UINT_PTR APIENTRY
    OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

    static int CALLBACK
    BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};

///////////////////////////////////////////////////////////////////////////////

class MFileSaveDialog : public IFileSaveDialog
{
public:
    static MFileSaveDialog *CreateInstance();

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IModalWindow interface
    STDMETHODIMP Show(HWND hwndOwner);

    // IFileDialog interface
    STDMETHODIMP SetFileTypes(
        UINT cFileTypes,
        const COMDLG_FILTERSPEC *rgFilterSpec);
    STDMETHODIMP SetFileTypeIndex(UINT iFileType);
    STDMETHODIMP GetFileTypeIndex(UINT *piFileType);
    STDMETHODIMP Advise(
        IFileDialogEvents *pfde,
        DWORD *pdwCookie);
    STDMETHODIMP Unadvise(DWORD dwCookie);
    STDMETHODIMP SetOptions(FILEOPENDIALOGOPTIONS fos);
    STDMETHODIMP GetOptions(FILEOPENDIALOGOPTIONS *pfos);
    STDMETHODIMP SetDefaultFolder(IShellItem *psi);
    STDMETHODIMP SetFolder(IShellItem *psi);
    STDMETHODIMP GetFolder(IShellItem **ppsi);
    STDMETHODIMP GetCurrentSelection(IShellItem **ppsi);
    STDMETHODIMP SetFileName(LPCWSTR pszName);
    STDMETHODIMP GetFileName(LPWSTR *pszName);
    STDMETHODIMP SetTitle(LPCWSTR pszTitle);
    STDMETHODIMP SetOkButtonLabel(LPCWSTR pszText);
    STDMETHODIMP SetFileNameLabel(LPCWSTR pszLabel);
    STDMETHODIMP GetResult(IShellItem **ppsi);
    STDMETHODIMP AddPlace(IShellItem *psi, FDAP fdap);
    STDMETHODIMP SetDefaultExtension(LPCWSTR pszDefaultExtension);
    STDMETHODIMP Close(HRESULT hr);
    STDMETHODIMP SetClientGuid(REFGUID guid);
    STDMETHODIMP ClearClientData();
    STDMETHODIMP SetFilter(IShellItemFilter *pFilter);

    // IFileSaveDialog interface
    STDMETHODIMP SetSaveAsItem(IShellItem *psi);
    STDMETHODIMP SetProperties(IPropertyStore *pStore);
    STDMETHODIMP SetCollectedProperties(
        IPropertyDescriptionList *pList,
        BOOL fAppendDefault);
    STDMETHODIMP GetProperties(IPropertyStore **ppStore);
    STDMETHODIMP ApplyProperties(
        IShellItem *psi,
        IPropertyStore *pStore,
        HWND hwnd,
        IFileOperationProgressSink *pSink);

protected:
    LONG m_nRefCount;
    HWND m_hwnd;
    LPITEMIDLIST m_pidlSelected;
    LPITEMIDLIST m_pidlDefFolder;
    BOOL m_bDoSave;
    FILEOPENDIALOGOPTIONS m_options;
    LPWSTR m_pszzFiles;
    LPWSTR m_pszTitle;
    LPWSTR m_pszzFilter;
    IFileDialogEvents *m_pEvents;
    DWORD m_dwCookie;
    WCHAR m_szFile[MAX_PATH];
    WCHAR m_szDefExt[32];
    OPENFILENAMEW m_ofn;
    BROWSEINFO m_bi;

    MFileSaveDialog();
    virtual ~MFileSaveDialog();
    void Init();

    BOOL UpdateFlags();
    BOOL IsFolderDialog() const;
    LPITEMIDLIST GetFolderIDList();
    BOOL VerifyFileType();

    static UINT_PTR APIENTRY
    OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

    static int CALLBACK
    BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};

///////////////////////////////////////////////////////////////////////////////

IFileOpenDialog *createFileOpenDialog(void)
{
    return MFileOpenDialog::CreateInstance();
}

IFileSaveDialog *createFileSaveDialog(void)
{
    return MFileSaveDialog::CreateInstance();
}

///////////////////////////////////////////////////////////////////////////////
// MFileOpenDialog implementation

#undef THIS_CLASS
#define THIS_CLASS MFileOpenDialog
#include "FileDialog_common.hpp"

void THIS_CLASS::Init()
{
    m_bDoSave = FALSE;
    m_options = FOS_NOCHANGEDIR | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST;
    m_ofn.Flags |= OFN_HIDEREADONLY;
}

STDMETHODIMP THIS_CLASS::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown_) ||
        IsEqualIID(riid, IID_IFileDialog_) ||
        IsEqualIID(riid, IID_IModalWindow_) ||
        IsEqualIID(riid, IID_IFileOpenDialog_))
    {
        *ppvObj = static_cast<IFileOpenDialog *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

// IFileOpenDialog interface

STDMETHODIMP THIS_CLASS::GetResults(IShellItemArray **ppenum)
{
    if (!ppenum || !(m_options & FOS_ALLOWMULTISELECT))
        return E_INVALIDARG;

    if (!m_pszzFiles || !*m_pszzFiles)
        return E_FAIL;

    MShellItemArray *pArray = MShellItemArray::CreateInstance();
    if (!pArray)
    {
        return E_OUTOFMEMORY;
    }

    WCHAR szPath[MAX_PATH];
    LPWSTR pchTitle = NULL;
    for (LPWSTR pch = m_pszzFiles; pch && *pch; pch += lstrlenW(pch) + 1)
    {
        if (pch == m_pszzFiles)
        {
            StringCbCopyW(szPath, sizeof(szPath), pch);
            pchTitle = szPath + lstrlenW(szPath);
            continue;
        }
        else
        {
            *pchTitle = 0;
            PathAppendW(szPath, pch);
        }

        LPITEMIDLIST pidl = ILCreateFromPathW(szPath);
        IShellItem *psi = NULL;
        SHCreateShellItemForXP0(NULL, NULL, pidl, &psi);
        CoTaskMemFree(pidl);

        if (!psi)
        {
            pArray->Release();
            return E_OUTOFMEMORY;
        }

        pArray->AddItem(psi);
        psi->Release();
    }

    *ppenum = pArray;

    return S_OK;
}

STDMETHODIMP THIS_CLASS::GetSelectedItems(IShellItemArray **ppsai)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
// MFileSaveDialog implementation

#undef THIS_CLASS
#define THIS_CLASS MFileSaveDialog
#include "FileDialog_common.hpp"

void THIS_CLASS::Init()
{
    m_bDoSave = TRUE;
    m_options = FOS_OVERWRITEPROMPT | FOS_NOCHANGEDIR |
                FOS_PATHMUSTEXIST | FOS_NOREADONLYRETURN;
}

// IUnknown interface

STDMETHODIMP THIS_CLASS::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown_) ||
        IsEqualIID(riid, IID_IFileDialog_) ||
        IsEqualIID(riid, IID_IModalWindow_) ||
        IsEqualIID(riid, IID_IFileSaveDialog_))
    {
        *ppvObj = static_cast<IFileSaveDialog *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

// IFileSaveDialog interface

STDMETHODIMP THIS_CLASS::SetSaveAsItem(IShellItem *psi)
{
    if (!psi)
        return E_INVALIDARG;

    LPITEMIDLIST pidl = NULL;
    SHGetIDListFromObjectForXP(psi, &pidl);

    WCHAR szPath[MAX_PATH];
    SHGetPathFromIDListW(pidl, szPath);
    SetFileName(szPath);

    return S_OK;
}

STDMETHODIMP THIS_CLASS::SetProperties(IPropertyStore *pStore)
{
    return E_NOTIMPL;
}

STDMETHODIMP THIS_CLASS::SetCollectedProperties(
    IPropertyDescriptionList *pList,
    BOOL fAppendDefault)
{
    return E_NOTIMPL;
}

STDMETHODIMP THIS_CLASS::GetProperties(IPropertyStore **ppStore)
{
    return E_NOTIMPL;
}

STDMETHODIMP THIS_CLASS::ApplyProperties(
    IShellItem *psi,
    IPropertyStore *pStore,
    HWND hwnd,
    IFileOperationProgressSink *pSink)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////

#undef THIS_CLASS
