/****************************************************************************
*                                                                           *
* Project64 - A Nintendo 64 emulator.                                      *
* http://www.pj64-emu.com/                                                  *
* Copyright (C) 2012 Project64. All rights reserved.                        *
*                                                                           *
* License:                                                                  *
* GNU/GPLv2 http://www.gnu.org/licenses/gpl-2.0.html                        *
*                                                                           *
****************************************************************************/

#include "stdafx.h"
#include "DebuggerUI.h"

#include "Symbols.h"
#include "Breakpoints.h"
#include "Assembler.h"

#include <Project64-core/N64System/Mips/OpCodeName.h>

static INT_PTR CALLBACK TabProcGPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK TabProcFPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK TabProcPI(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK TabProcCOP0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD CDebugCommandsView::GPREditIds[32] = {
	IDC_R0_EDIT,  IDC_R1_EDIT,  IDC_R2_EDIT,  IDC_R3_EDIT,
	IDC_R4_EDIT,  IDC_R5_EDIT,  IDC_R6_EDIT,  IDC_R7_EDIT,
	IDC_R8_EDIT,  IDC_R9_EDIT,  IDC_R10_EDIT, IDC_R11_EDIT,
	IDC_R12_EDIT, IDC_R13_EDIT, IDC_R14_EDIT, IDC_R15_EDIT,
	IDC_R16_EDIT, IDC_R17_EDIT, IDC_R18_EDIT, IDC_R19_EDIT,
	IDC_R20_EDIT, IDC_R21_EDIT, IDC_R22_EDIT, IDC_R23_EDIT,
	IDC_R24_EDIT, IDC_R25_EDIT, IDC_R26_EDIT, IDC_R27_EDIT,
	IDC_R28_EDIT, IDC_R29_EDIT, IDC_R30_EDIT, IDC_R31_EDIT
};

DWORD CDebugCommandsView::FPREditIds[32] = {
	IDC_F0_EDIT,  IDC_F1_EDIT,  IDC_F2_EDIT,  IDC_F3_EDIT,
	IDC_F4_EDIT,  IDC_F5_EDIT,  IDC_F6_EDIT,  IDC_F7_EDIT,
	IDC_F8_EDIT,  IDC_F9_EDIT,  IDC_F10_EDIT, IDC_F11_EDIT,
	IDC_F12_EDIT, IDC_F13_EDIT, IDC_F14_EDIT, IDC_F15_EDIT,
	IDC_F16_EDIT, IDC_F17_EDIT, IDC_F18_EDIT, IDC_F19_EDIT,
	IDC_F20_EDIT, IDC_F21_EDIT, IDC_F22_EDIT, IDC_F23_EDIT,
	IDC_F24_EDIT, IDC_F25_EDIT, IDC_F26_EDIT, IDC_F27_EDIT,
	IDC_F28_EDIT, IDC_F29_EDIT, IDC_F30_EDIT, IDC_F31_EDIT
};

DWORD CDebugCommandsView::PIEditIds[13] = {
	IDC_PI00_EDIT, IDC_PI04_EDIT, IDC_PI08_EDIT, IDC_PI0C_EDIT,
	IDC_PI10_EDIT, IDC_PI14_EDIT, IDC_PI18_EDIT, IDC_PI1C_EDIT,
	IDC_PI20_EDIT, IDC_PI24_EDIT, IDC_PI28_EDIT, IDC_PI2C_EDIT,
	IDC_PI30_EDIT
};

DWORD CDebugCommandsView::COP0EditIds[19] = {
	IDC_COP0_0_EDIT,  IDC_COP0_1_EDIT,  IDC_COP0_2_EDIT,  IDC_COP0_3_EDIT,
	IDC_COP0_4_EDIT,  IDC_COP0_5_EDIT,  IDC_COP0_6_EDIT,  IDC_COP0_7_EDIT,
	IDC_COP0_8_EDIT,  IDC_COP0_9_EDIT,  IDC_COP0_10_EDIT, IDC_COP0_11_EDIT,
	IDC_COP0_12_EDIT, IDC_COP0_13_EDIT, IDC_COP0_14_EDIT, IDC_COP0_15_EDIT,
	IDC_COP0_16_EDIT, IDC_COP0_17_EDIT, IDC_COP0_18_EDIT,
};

int CDebugCommandsView::MapGPREdit(DWORD controlId)
{
	for (int i = 0; i < 32; i++)
	{
		if (GPREditIds[i] == controlId)
		{
			return i;
		}
	}
	return -1;
}

int CDebugCommandsView::MapFPREdit(DWORD controlId)
{
	for (int i = 0; i < 32; i++)
	{
		if (FPREditIds[i] == controlId)
		{
			return i;
		}
	}
	return -1;
}

int CDebugCommandsView::MapPIEdit(DWORD controlId)
{
	for (int i = 0; i < 13; i++)
	{
		if (PIEditIds[i] == controlId)
		{
			return i;
		}
	}
	return -1;
}

int CDebugCommandsView::MapCOP0Edit(DWORD controlId)
{
	for (int i = 0; i < 19; i++)
	{
		if (COP0EditIds[i] == controlId)
		{
			return i;
		}
	}
	return -1;
}

CDebugCommandsView::CDebugCommandsView(CDebuggerUI * debugger) :
CDebugDialog<CDebugCommandsView>(debugger)
{
	m_bIngoreAddrChange = false;
	m_StartAddress = 0x80000000;
	m_Breakpoints = m_Debugger->Breakpoints();
	m_bEditing = false;
}

CDebugCommandsView::~CDebugCommandsView(void)
{

}

LRESULT	CDebugCommandsView::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DlgResize_Init(false, false);

	m_ptMinTrackSize.x = 580;
	m_ptMinTrackSize.y = 490;

	m_CommandListRows = 45;

	CheckCPUType();
	
	GetWindowRect(&m_DefaultWindowRect);

	// Setup address input

	m_AddressEdit.Attach(GetDlgItem(IDC_ADDR_EDIT));
	m_AddressEdit.SetDisplayType(CEditNumber::DisplayHex);
	m_AddressEdit.SetLimitText(8);

	// Setup register tabs & inputs

	m_RegisterTabs.Attach(GetDlgItem(IDC_REG_TABS));

	m_RegisterTabs.ResetTabs();

	m_GPRTab = m_RegisterTabs.AddTab("GPR", IDD_Debugger_GPR, TabProcGPR);
	m_FPRTab = m_RegisterTabs.AddTab("FPR", IDD_Debugger_FPR, TabProcFPR);
	m_PITab  = m_RegisterTabs.AddTab("PI",  IDD_Debugger_PI,  TabProcPI);
	m_COP0Tab = m_RegisterTabs.AddTab("COP0", IDD_Debugger_COP0, TabProcCOP0);

	HFONT monoFont = CreateFont(-11, 0, 0, 0,
		FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, FF_DONTCARE, "Consolas"
	);

	for (int i = 0; i < 32; i++)
	{
		m_GPREdits[i].Attach(m_GPRTab.GetDlgItem(GPREditIds[i]));
		m_GPREdits[i].SetFont(monoFont, FALSE);

		m_FPREdits[i].Attach(m_FPRTab.GetDlgItem(FPREditIds[i]));
		m_FPREdits[i].SetDisplayType(CEditNumber::DisplayHex);
		m_FPREdits[i].SetFont(monoFont, FALSE);
	}

	for (int i = 0; i < 13; i++)
	{
		m_PIEdits[i].Attach(m_PITab.GetDlgItem(PIEditIds[i]));
		m_PIEdits[i].SetDisplayType(CEditNumber::DisplayHex);
		m_PIEdits[i].SetFont(monoFont, FALSE);
	}

	for (int i = 0; i < 19; i++)
	{
		m_COP0Edits[i].Attach(m_COP0Tab.GetDlgItem(COP0EditIds[i]));
		m_COP0Edits[i].SetDisplayType(CEditNumber::DisplayHex);
		m_COP0Edits[i].SetFont(monoFont, FALSE);
	}

	m_HIEdit.Attach(m_GPRTab.GetDlgItem(IDC_HI_EDIT));
	m_HIEdit.SetFont(monoFont, FALSE);

	m_LOEdit.Attach(m_GPRTab.GetDlgItem(IDC_LO_EDIT));
	m_LOEdit.SetFont(monoFont, FALSE);

	RefreshRegisterEdits();

	// Setup breakpoint list

	m_BreakpointList.Attach(GetDlgItem(IDC_BP_LIST));
	m_BreakpointList.ModifyStyle(NULL, LBS_NOTIFY);
	RefreshBreakpointList();

	// Setup list scrollbar

	m_Scrollbar.Attach(GetDlgItem(IDC_SCRL_BAR));
	m_Scrollbar.SetScrollRange(0, 100, FALSE);
	m_Scrollbar.SetScrollPos(50, TRUE);
	//m_Scrollbar.GetScrollInfo(); // todo bigger thumb size
	//m_Scrollbar.SetScrollInfo();

	// Setup command list
	m_CommandList.Attach(GetDlgItem(IDC_CMD_LIST));
	m_CommandList.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);
	m_CommandList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_CommandList.AddColumn("Address", 0);
	m_CommandList.AddColumn("Command", 1);
	m_CommandList.AddColumn("Parameters", 2);
	m_CommandList.AddColumn("Symbol", 3);
	m_CommandList.SetColumnWidth(0, 60);
	m_CommandList.SetColumnWidth(1, 60);
	m_CommandList.SetColumnWidth(2, 120);
	m_CommandList.SetColumnWidth(3, 120);

	// Op editor
	m_OpEdit.Attach(GetDlgItem(IDC_OP_EDIT));
	m_OpEdit.SetCommandsWindow(this);
	m_OpEdit.ShowWindow(SW_HIDE);
	
	// Setup stack list
	m_StackList.Attach(GetDlgItem(IDC_STACK_LIST));
	m_StackList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_StackList.AddColumn("#", 0);
	m_StackList.AddColumn("00", 1);
	m_StackList.AddColumn("04", 2);
	m_StackList.AddColumn("08", 3);
	m_StackList.AddColumn("0C", 4);

	m_StackList.SetColumnWidth(0, 22);
	m_StackList.SetColumnWidth(1, 64);
	m_StackList.SetColumnWidth(2, 64);
	m_StackList.SetColumnWidth(3, 64);
	m_StackList.SetColumnWidth(4, 64);
	
	RefreshStackList();
	
	ShowAddress(0x80000000, FALSE);

	WindowCreated();
	return TRUE;
}

LRESULT CDebugCommandsView::OnDestroy(void)
{
	return 0;
}

LRESULT	CDebugCommandsView::OnOpKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == VK_UP)
	{
		m_SelectedAddress -= 4;
		BeginOpEdit(m_SelectedAddress);
		bHandled = TRUE;
	}
	else if (wParam == VK_DOWN)
	{
		m_SelectedAddress += 4;
		BeginOpEdit(m_SelectedAddress);
		bHandled = TRUE;
	}
	else if (wParam == VK_RETURN)
	{
		int textLen = m_OpEdit.GetWindowTextLengthA();
		char text[256];
		m_OpEdit.GetWindowTextA(text, 255);
		m_OpEdit.SetWindowTextA("");
		uint32_t op;
		bool bValid = CAssembler::AssembleLine(text, &op, m_SelectedAddress);
		if (bValid)
		{
			EditOp(m_SelectedAddress, op);
			m_SelectedAddress += 4;
			BeginOpEdit(m_SelectedAddress);
		}
		bHandled = TRUE;
	}
	else if (wParam == VK_ESCAPE)
	{
		EndOpEdit();
		bHandled = TRUE;
	}
	return 1;
}

/*
LRESULT CDebugCommandsView::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// Allow only vertical resizing
	MINMAXINFO* minMax = (MINMAXINFO*)lParam;
	minMax->ptMinTrackSize.x = m_DefaultWindowRect.Width();
	minMax->ptMaxTrackSize.x = m_DefaultWindowRect.Width();
	minMax->ptMinTrackSize.y = m_DefaultWindowRect.Height();
	return FALSE;
}*/

void CDebugCommandsView::CheckCPUType()
{
	CPU_TYPE cpuType;

	if (g_Settings->LoadBool(Setting_ForceInterpreterCPU))
	{
		cpuType = CPU_Interpreter;
	}
	else
	{
		cpuType = g_System->CpuType();
	}
	
	if (cpuType != CPU_Interpreter)
	{
		MessageBox("Interpreter mode required", "Invalid CPU Type", MB_OK);
	}
}

// Check if KSEG0 addr is out of bounds
bool CDebugCommandsView::AddressSafe(uint32_t vaddr)
{
	if (g_MMU == NULL)
	{
		return false;
	}

	if (vaddr >= 0x80000000 && vaddr <= 0x9FFFFFFF)
	{
		if ((vaddr & 0x1FFFFFFF) >= g_MMU->RdramSize())
		{
			return false;
		}
	}

	return true;
}

void CDebugCommandsView::ShowAddress(DWORD address, BOOL top)
{
	if (top == TRUE)
	{
		m_StartAddress = address;
	}
	else if (address < m_StartAddress || address > m_StartAddress + (m_CommandListRows - 1) * 4)
	{
		// change start address if out of view
		m_StartAddress = address;
		m_bIngoreAddrChange = true;
		m_AddressEdit.SetValue(address, false, true);
	}
	
	m_CommandList.SetRedraw(FALSE);
	m_CommandList.DeleteAllItems();

	char addrStr[9];

	for (int i = 0; i < m_CommandListRows; i++)
	{	
		uint32_t opAddr = m_StartAddress + i * 4;

		sprintf(addrStr, "%08X", opAddr);
		
		m_CommandList.AddItem(i, 0, addrStr);

		OPCODE OpCode;
		bool bAddrOkay = false;

		if (AddressSafe(opAddr))
		{
			bAddrOkay = g_MMU->LW_VAddr(opAddr, OpCode.Hex);
		}

		if(!bAddrOkay)
		{
			m_CommandList.AddItem(i, 1, "***");
			continue;
		}
		
		char* command = (char*)R4300iOpcodeName(OpCode.Hex, opAddr);
		char* cmdName = strtok((char*)command, "\t");
		char* cmdArgs = strtok(NULL, "\t");

		if (strcmp(cmdName, "JAL") == 0)
		{
			uint32_t targetAddr = (0x80000000 | (OpCode.target << 2));

			// todo move symbols management to CDebuggerUI
			const char* targetSymbolName = CSymbols::GetNameByAddress(targetAddr);
			if (targetSymbolName != NULL)
			{
				cmdArgs = (char*)targetSymbolName;
			}
		}
		
		m_CommandList.AddItem(i, 1, cmdName);
		m_CommandList.AddItem(i, 2, cmdArgs);

		const char* targetSymbolName = CSymbols::GetNameByAddress(opAddr);
		if (targetSymbolName != NULL)
		{
			m_CommandList.AddItem(i, 3, targetSymbolName);
		}

	}
	
	if (!top) // update registers & stack when called via breakpoint/stepping
	{
		RefreshRegisterEdits();
		RefreshStackList();
	}

	RefreshBreakpointList();
	
	m_CommandList.SetRedraw(TRUE);
	m_CommandList.RedrawWindow();
}

void CDebugCommandsView::RefreshRegisterEdits()
{
	//registersUpdating = TRUE;
	if (g_Reg != NULL) {
		char regText[9];
		for (int i = 0; i < 32; i++)
		{
			m_GPREdits[i].SetValue(g_Reg->m_GPR[i].UDW);
			m_FPREdits[i].SetValue(*(uint32_t *)g_Reg->m_FPR_S[i], false, true);
		}
		m_HIEdit.SetValue(g_Reg->m_HI.UDW);
		m_LOEdit.SetValue(g_Reg->m_LO.UDW);
		
		m_PIEdits[0].SetValue(g_Reg->PI_DRAM_ADDR_REG, false, true);
		m_PIEdits[1].SetValue(g_Reg->PI_CART_ADDR_REG, false, true);
		m_PIEdits[2].SetValue(g_Reg->PI_RD_LEN_REG, false, true);
		m_PIEdits[3].SetValue(g_Reg->PI_WR_LEN_REG, false, true);
		m_PIEdits[4].SetValue(g_Reg->PI_STATUS_REG, false, true);
		m_PIEdits[5].SetValue(g_Reg->PI_BSD_DOM1_LAT_REG, false, true);
		m_PIEdits[6].SetValue(g_Reg->PI_BSD_DOM1_PWD_REG, false, true);
		m_PIEdits[7].SetValue(g_Reg->PI_BSD_DOM1_PGS_REG, false, true);
		m_PIEdits[8].SetValue(g_Reg->PI_BSD_DOM1_RLS_REG, false, true);
		m_PIEdits[9].SetValue(g_Reg->PI_BSD_DOM2_LAT_REG, false, true);
		m_PIEdits[10].SetValue(g_Reg->PI_BSD_DOM2_PWD_REG, false, true);
		m_PIEdits[11].SetValue(g_Reg->PI_BSD_DOM2_PGS_REG, false, true);
		m_PIEdits[12].SetValue(g_Reg->PI_BSD_DOM2_RLS_REG, false, true);

		m_COP0Edits[0].SetValue(g_Reg->INDEX_REGISTER, false, true);
		m_COP0Edits[1].SetValue(g_Reg->RANDOM_REGISTER, false, true);
		m_COP0Edits[2].SetValue(g_Reg->ENTRYLO0_REGISTER, false, true);
		m_COP0Edits[3].SetValue(g_Reg->ENTRYLO1_REGISTER, false, true);
		m_COP0Edits[4].SetValue(g_Reg->CONTEXT_REGISTER, false, true);
		m_COP0Edits[5].SetValue(g_Reg->PAGE_MASK_REGISTER, false, true);
		m_COP0Edits[6].SetValue(g_Reg->WIRED_REGISTER, false, true);
		m_COP0Edits[7].SetValue(g_Reg->BAD_VADDR_REGISTER, false, true);
		m_COP0Edits[8].SetValue(g_Reg->COUNT_REGISTER, false, true);
		m_COP0Edits[9].SetValue(g_Reg->ENTRYHI_REGISTER, false, true);
		m_COP0Edits[10].SetValue(g_Reg->COMPARE_REGISTER, false, true);
		m_COP0Edits[11].SetValue(g_Reg->STATUS_REGISTER, false, true);
		m_COP0Edits[12].SetValue(g_Reg->CAUSE_REGISTER, false, true);
		m_COP0Edits[13].SetValue(g_Reg->EPC_REGISTER, false, true);
		m_COP0Edits[14].SetValue(g_Reg->CONFIG_REGISTER, false, true);
		m_COP0Edits[15].SetValue(g_Reg->TAGLO_REGISTER, false, true);
		m_COP0Edits[16].SetValue(g_Reg->TAGHI_REGISTER, false, true);
		m_COP0Edits[17].SetValue(g_Reg->ERROREPC_REGISTER, false, true);
		m_COP0Edits[18].SetValue(g_Reg->FAKE_CAUSE_REGISTER, false, true);
	}
	else
	{
		for (int i = 0; i < 32; i++)
		{
			m_GPREdits[i].SetValue(0);
			m_FPREdits[i].SetWindowTextA("00000000");
		}

		for (int i = 0; i < 13; i++)
		{
			m_PIEdits[i].SetWindowTextA("00000000");
		}

		for (int i = 0; i < 19; i++)
		{
			m_PIEdits[i].SetWindowTextA("00000000");
		}
		m_HIEdit.SetValue(0);
		m_LOEdit.SetValue(0);
	}
	//registersUpdating = FALSE;
}

void CDebugCommandsView::RefreshBreakpointList()
{
	m_BreakpointList.ResetContent();
	char rowStr[16];
	for (int i = 0; i < m_Breakpoints->m_nRBP; i++)
	{
		sprintf(rowStr, "R %08X", m_Breakpoints->m_RBP[i]);
		int index = m_BreakpointList.AddString(rowStr);
		m_BreakpointList.SetItemData(index, m_Breakpoints->m_RBP[i]);
	}
	for (int i = 0; i < m_Breakpoints->m_nWBP; i++)
	{
		sprintf(rowStr, "W %08X", m_Breakpoints->m_WBP[i]);
		int index = m_BreakpointList.AddString(rowStr);
		m_BreakpointList.SetItemData(index, m_Breakpoints->m_WBP[i]);
	}
	for (int i = 0; i < m_Breakpoints->m_nEBP; i++)
	{
		sprintf(rowStr, "E %08X", m_Breakpoints->m_EBP[i]);
		int index = m_BreakpointList.AddString(rowStr);
		m_BreakpointList.SetItemData(index, m_Breakpoints->m_EBP[i]);
	}
}

void CDebugCommandsView::RefreshStackList()
{
	m_StackList.SetRedraw(FALSE);
	m_StackList.DeleteAllItems();

	uint32_t spBase;
	
	if (g_Reg)
	{
		spBase = g_Reg->m_GPR[29].UW[0];
	}

	for (int i = 0; i < 0x10; i++)
	{
		char t[4];
		sprintf(t, "%02X", i * 0x10);
		m_StackList.AddItem(i, 0, t);

		for (int j = 0; j < 4; j++)
		{
			if (g_MMU == NULL)
			{
				m_StackList.AddItem(i, j + 1, "????????");
				continue;
			}

			uint32_t val;
			g_MMU->LW_VAddr(spBase + i * 0x10 + j * 4, val);

			char valStr[9];
			sprintf(valStr, "%08X", val);
			m_StackList.AddItem(i, j + 1, valStr);
		}
	}

	m_StackList.SetRedraw(TRUE);
}

void CDebugCommandsView::RemoveSelectedBreakpoints()
{
	int nItem = m_BreakpointList.GetCurSel();
	
	if (nItem == LB_ERR)
	{
		return;
	}

	char itemText[32];
	m_BreakpointList.GetText(nItem, itemText);

	uint32_t address = m_BreakpointList.GetItemData(nItem);

	switch (itemText[0])
	{
	case 'E':
		m_Breakpoints->EBPRemove(address);
		break;
	case 'W':
		m_Breakpoints->WBPRemove(address);
		break;
	case 'R':
		m_Breakpoints->RBPRemove(address);
		break;
	}

	RefreshBreakpointList();
}

LRESULT	CDebugCommandsView::OnMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// Set command list row height to 13px
	if(wParam == IDC_CMD_LIST)
	{
		MEASUREITEMSTRUCT* lpMeasureItem = (MEASUREITEMSTRUCT*)lParam;
		lpMeasureItem->itemHeight = 13;
	}
	return FALSE;
}


LRESULT CDebugCommandsView::OnCustomDrawList(NMHDR* pNMHDR)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	DWORD drawStage = pLVCD->nmcd.dwDrawStage;
	
	switch (drawStage)
	{
	case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
	case (CDDS_ITEMPREPAINT | CDDS_SUBITEM): break;
	default: return CDRF_DODEFAULT;
	}

	DWORD nItem = pLVCD->nmcd.dwItemSpec;
	DWORD nSubItem = pLVCD->iSubItem;
	
	uint32_t address = m_StartAddress + (nItem * 4);
	uint32_t pc = (g_Reg != NULL) ? g_Reg->m_PROGRAM_COUNTER : 0;

	OPCODE pcOpcode;
	if (g_MMU != NULL)
	{
		g_MMU->LW_VAddr(pc, pcOpcode.Hex);
	}

	if (nSubItem == 0) // addr
	{
		if (m_Breakpoints->EBPExists(address))
		{
			// breakpoint
			pLVCD->clrTextBk = RGB(0x44, 0x00, 0x00);
			pLVCD->clrText = (address == pc) ?
				RGB(0xFF, 0xFF, 0x00) : // breakpoint & current pc
				RGB(0xFF, 0xCC, 0xCC);
		}
		else if (address == pc)
		{
			// pc
			pLVCD->clrTextBk = RGB(0x88, 0x88, 0x88);
			pLVCD->clrText = RGB(0xFF, 0xFF, 0);
		}
		else
		{
			//default
			pLVCD->clrTextBk = RGB(0xEE, 0xEE, 0xEE);
			pLVCD->clrText = RGB(0x44, 0x44, 0x44);
		}

		return CDRF_DODEFAULT;
	}
	
	// (nSubItem == 1 || nSubItem == 2) 

	// cmd & args
	OPCODE Opcode;
	bool bAddrOkay = false;

	if (AddressSafe(address))
	{
		bAddrOkay = g_MMU->LW_VAddr(address, Opcode.Hex);
	}
	
	if (!bAddrOkay)
	{
		// unmapped/invalid
		pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0xFF);
		pLVCD->clrText = RGB(0xFF, 0x00, 0x00);
	}
	else if (pc == address)
	{
		//pc
		pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0xAA);
		pLVCD->clrText = RGB(0x22, 0x22, 0);
	}
	else if (IsOpEdited(address))
	{
		// red
		pLVCD->clrTextBk = RGB(0xFF, 0xEE, 0xFF);
		pLVCD->clrText = RGB(0xFF, 0x00, 0xFF);
	}
	else if (Opcode.op == R4300i_ADDIU && Opcode.rt == 29) // stack shift
	{
		if ((short)Opcode.immediate < 0) // alloc
		{
			// sky blue bg, dark blue fg
			pLVCD->clrTextBk = RGB(0xCC, 0xDD, 0xFF);
			pLVCD->clrText = RGB(0x00, 0x11, 0x44);
		}
		else // free
		{
			// salmon bg, dark red fg
			pLVCD->clrTextBk = RGB(0xFF, 0xDD, 0xDD);
			pLVCD->clrText = RGB(0x44, 0x00, 0x00);
		}
	}
	else if (Opcode.Hex == 0x00000000) // nop
	{
		// gray fg
		pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0xFF);
		pLVCD->clrText = RGB(0x88, 0x88, 0x88);
	}
	else if (Opcode.op == R4300i_J || Opcode.op == R4300i_JAL || (Opcode.op == 0 && Opcode.funct == R4300i_SPECIAL_JR))
	{
		// jumps
		pLVCD->clrText = RGB(0x00, 0x66, 0x00);
		pLVCD->clrTextBk = RGB(0xF5, 0xFF, 0xF5);
	}
	else
	{
		pLVCD->clrTextBk = RGB(0xFF, 0xFF, 0xFF);
		pLVCD->clrText = RGB(0x00, 0x00, 0x00);
	}

	if (!m_Breakpoints->isDebugging())
	{
		return CDRF_DODEFAULT;
	}

	// color register usage
	// todo localise to temp register context (dont look before/after jumps and frame shifts)
	COLORREF clrUsedRegister = RGB(0xE5, 0xE0, 0xFF); // light purple
	COLORREF clrAffectedRegister = RGB(0xFF, 0xE0, 0xFF); // light pink

	int pcUsedRegA = 0, pcUsedRegB = 0, pcChangedReg = 0;
	int curUsedRegA = 0, curUsedRegB = 0, curChangedReg = 0;
	
	if (pcOpcode.op == R4300i_SPECIAL)
	{
		pcUsedRegA = pcOpcode.rs;
		pcUsedRegB = pcOpcode.rt;
		pcChangedReg = pcOpcode.rd;
	}
	else
	{
		pcUsedRegA = pcOpcode.rs;
		pcChangedReg = pcOpcode.rt;
	}

	if (Opcode.op == R4300i_SPECIAL)
	{
		curUsedRegA = Opcode.rs;
		curUsedRegB = Opcode.rt;
		curChangedReg = Opcode.rd;
	}
	else
	{
		curUsedRegA = Opcode.rs;
		curChangedReg = Opcode.rt;
	}

	if (address < pc)
	{
		if (curChangedReg != 0 && (pcUsedRegA == curChangedReg || pcUsedRegB == curChangedReg))
		{
			pLVCD->clrTextBk = clrUsedRegister;
		}
	}
	else if (address > pc)
	{
		if (pcChangedReg != 0 && (curUsedRegA == pcChangedReg || curUsedRegB == pcChangedReg))
		{
			pLVCD->clrTextBk = clrAffectedRegister;
		}
	}
	return CDRF_DODEFAULT;
}

LRESULT CDebugCommandsView::OnClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	switch (wID)
	{
	case IDC_SYMBOLS_BTN:
		m_Debugger->Debug_ShowSymbolsWindow();
		break;
	case IDC_GO_BTN:
		m_Breakpoints->StopDebugging();
		m_Breakpoints->StopDebugging();
		m_Breakpoints->Resume();
		break;
	case IDC_STEP_BTN:
		m_Breakpoints->KeepDebugging();
		m_Breakpoints->Resume();
		break;
	case IDC_SKIP_BTN:
		m_Breakpoints->KeepDebugging();
		m_Breakpoints->Skip();
		m_Breakpoints->Resume();
		break;
	case IDC_CLEARBP_BTN:
		m_Breakpoints->BPClear();
		RefreshBreakpointList();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case IDC_ADDBP_BTN:
		m_AddBreakpointDlg.DoModal(m_Debugger);
		RefreshBreakpointList();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case IDC_RMBP_BTN:
		RemoveSelectedBreakpoints();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case IDCANCEL:
		EndDialog(0);
		break;
	//popup
	case ID_POPUPMENU_EDIT:
		BeginOpEdit(m_SelectedAddress);
		break;
	case ID_POPUPMENU_INSERTNOP:
		EditOp(m_SelectedAddress, 0x00000000);
		ShowAddress(m_StartAddress, TRUE);
		break;
	case ID_POPUPMENU_RESTORE:
		RestoreOp(m_SelectedAddress);
		ShowAddress(m_StartAddress, TRUE);
		break;
	case ID_POPUPMENU_RESTOREALL:
		RestoreAllOps();
		ShowAddress(m_StartAddress, TRUE);
		break;
	case ID_POPUPMENU_ADDSYMBOL:
		m_AddSymbolDlg.DoModal(m_Debugger, m_SelectedAddress, CSymbols::TYPE_CODE);
		break;
	}
	return FALSE;
}

LRESULT CDebugCommandsView::OnMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	uint32_t newAddress = m_StartAddress - ((short)HIWORD(wParam) / WHEEL_DELTA) * 4;
	
	m_StartAddress = newAddress;

	m_AddressEdit.SetValue(m_StartAddress, false, true);

	return TRUE;
}

void CDebugCommandsView::GotoEnteredAddress()
{
	char text[9];

	m_AddressEdit.GetWindowTextA(text, 9);

	DWORD address = strtoul(text, NULL, 16);
	address = address - address % 4;
	ShowAddress(address, TRUE);
}

void CDebugCommandsView::BeginOpEdit(uint32_t address)
{
	m_bEditing = true;
	ShowAddress(address, FALSE);
	int nItem = (address - m_StartAddress) / 4;
	CRect itemRect;
	m_CommandList.GetSubItemRect(nItem, 1, 0, &itemRect);
	//itemRect.bottom += 0;
	itemRect.left += 3;
	itemRect.right += 100;
	
	uint32_t opcode;
	g_MMU->LW_VAddr(address, opcode);
	char* command = (char*)R4300iOpcodeName(opcode, address);

	m_OpEdit.ShowWindow(SW_SHOW);
	m_OpEdit.MoveWindow(&itemRect);
	m_OpEdit.BringWindowToTop();
	m_OpEdit.SetWindowTextA(command);
	m_OpEdit.SetFocus();
	m_OpEdit.SetSelAll();

	m_CommandList.RedrawWindow();
	m_OpEdit.RedrawWindow();
}

void CDebugCommandsView::EndOpEdit()
{
	m_bEditing = false;
	m_OpEdit.SetWindowTextA("");
	m_OpEdit.ShowWindow(SW_HIDE);
}

LRESULT CDebugCommandsView::OnAddrChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_bIngoreAddrChange)
	{
		m_bIngoreAddrChange = false;
		return 0;
	}
	GotoEnteredAddress();
	return 0;
}

LRESULT	CDebugCommandsView::OnCommandListClicked(NMHDR* pNMHDR)
{
	EndOpEdit();
	return 0;
}

LRESULT	CDebugCommandsView::OnCommandListDblClicked(NMHDR* pNMHDR)
{
	// Set PC breakpoint
	NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	int nItem = pIA->iItem;
	
	uint32_t address = m_StartAddress + nItem * 4;
	if (m_Breakpoints->EBPExists(address))
	{
		m_Breakpoints->EBPRemove(address);
	}
	else
	{
		m_Breakpoints->EBPAdd(address);
	}
	// Cancel blue highlight
	m_AddressEdit.SetFocus();
	RefreshBreakpointList();

	return CDRF_DODEFAULT;
}

LRESULT	CDebugCommandsView::OnCommandListRightClicked(NMHDR* pNMHDR)
{
	EndOpEdit();

	NMITEMACTIVATE* pIA = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	int nItem = pIA->iItem;

	uint32_t address = m_StartAddress + nItem * 4;
	m_SelectedAddress = address;

	HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_OP_POPUP));
	HMENU hPopupMenu = GetSubMenu(hMenu, 0);

	POINT mouse;
	GetCursorPos(&mouse);

	TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, mouse.x, mouse.y, 0, m_hWnd, NULL);

	DestroyMenu(hMenu);
	
	return CDRF_DODEFAULT;
}


LRESULT CDebugCommandsView::OnListBoxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (wID == IDC_BP_LIST)
	{
		int index = m_BreakpointList.GetCaretIndex();
		uint32_t address = m_BreakpointList.GetItemData(index);
		int len = m_BreakpointList.GetTextLen(index);
		char* rowText = (char*)malloc(len + 1);
		rowText[len] = '\0';
		m_BreakpointList.GetText(index, rowText);
		if (*rowText == 'E')
		{
			ShowAddress(address, true);
		}
		else
		{
			m_Debugger->Debug_ShowMemoryLocation(address, true);
		}
		free(rowText);
	}
	return FALSE;
}

LRESULT CDebugCommandsView::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	WORD type = LOWORD(wParam);

	if (type == WA_INACTIVE)
	{
		return FALSE;
	}

	if (type == WA_CLICKACTIVE)
	{
		CheckCPUType();
	}

	ShowAddress(m_StartAddress, TRUE);

	return FALSE;
}


LRESULT	CDebugCommandsView::OnSizing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CRect rect;
	GetClientRect(&rect);
	int height = rect.Height();
	
	int rows = (height / 13) - 4; // 13 row height

	if (m_CommandListRows != rows)
	{
		m_CommandListRows = rows;
		ShowAddress(m_StartAddress, TRUE);
	}
	
	m_RegisterTabs.RedrawCurrentTab();

	return FALSE;
}

LRESULT CDebugCommandsView::OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	WORD type = LOWORD(wParam);

	switch(type)
	{
	case SB_LINEUP:
		ShowAddress(m_StartAddress - 8, TRUE);
		break;
	case SB_LINEDOWN:
		ShowAddress(m_StartAddress + 8, TRUE);
		break;
	case SB_THUMBTRACK:
		{
			//int scrollPos = HIWORD(wParam);
			//ShowAddress(m_StartAddress + (scrollPos - 50) * 4, TRUE);
		}
		break;
	}

	return FALSE;
}

void CDebugCommandsView::Reset()
{
	ClearEditedOps();
}

void CDebugCommandsView::ClearEditedOps()
{
	m_EditedOps.clear();
}

BOOL CDebugCommandsView::IsOpEdited(uint32_t address)
{
	for (int i = 0; i < m_EditedOps.size(); i++)
	{
		if (m_EditedOps[i].address == address)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void CDebugCommandsView::EditOp(uint32_t address, uint32_t op)
{
	uint32_t currentOp;
	g_MMU->LW_VAddr(address, currentOp);

	if (currentOp == op)
	{
		return;
	}

	g_MMU->SW_VAddr(address, op);

	if (!IsOpEdited(address))
	{
		m_EditedOps.push_back({ address, currentOp });
	}
}

void CDebugCommandsView::RestoreOp(uint32_t address)
{
	for (int i = 0; i < m_EditedOps.size(); i++)
	{
		if (m_EditedOps[i].address == address)
		{
			g_MMU->SW_VAddr(m_EditedOps[i].address, m_EditedOps[i].originalOp);
			m_EditedOps.erase(m_EditedOps.begin() + i);
			break;
		}
	}
}

void CDebugCommandsView::RestoreAllOps()
{
	int lastIndex = m_EditedOps.size() - 1;
	for (int i = lastIndex; i >= 0; i--)
	{
		g_MMU->SW_VAddr(m_EditedOps[i].address, m_EditedOps[i].originalOp);
		m_EditedOps.erase(m_EditedOps.begin() + i);
	}
}


// Add breakpoint dialog

void CRegisterTabs::ResetTabs()
{
	m_TabWindows.clear();
	m_TabIds.clear();
}

static INT_PTR CALLBACK TabProcGPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	if (msg == WM_INITDIALOG)
	{
		return TRUE;
	}

	/*
	if (msg == WM_CTLCOLOREDIT)
	{
		if (g_Reg == NULL)
		{
			return FALSE;
		}

		HDC hdc = (HDC)wParam;

		SetTextColor(hdc, signExt ? RGB(255, 0, 0) : RGB(0, 0, 0));
		SetBkColor(hdc, RGB(255,255,255));

		return (INT_PTR)CreateSolidBrush(RGB(255,255,255));
	}*/

	/*if (registersUpdating)
	{
		return FALSE;
	}*/

	if (msg != WM_COMMAND)
	{
		return FALSE;
	}

	WORD notification = HIWORD(wParam);
	
	if (notification == EN_KILLFOCUS)
	{
		CBreakpoints* breakpoints = ((CDebuggerUI*)g_Debugger)->Breakpoints();
		if (g_Reg == NULL || !breakpoints->isDebugging())
		{
			return FALSE;
		}
		
		int ctrlId = LOWORD(wParam);
		
		// (if IDC_PC_EDIT here with early ret)
		
		HWND ctrl = GetDlgItem(hDlg, ctrlId);
		char text[20];
		GetWindowText(ctrl, text, 20);

		uint64_t value = CEditReg64::ParseValue(text);

		if (ctrlId == IDC_HI_EDIT)
		{
			g_Reg->m_HI.UDW = value;
		}
		else if (ctrlId == IDC_LO_EDIT)
		{
			g_Reg->m_HI.UDW = value;
		}
		else
		{
			int nReg = CDebugCommandsView::MapGPREdit(ctrlId);
			g_Reg->m_GPR[nReg].UDW = value;
		}
	}

	return FALSE;
}

static INT_PTR CALLBACK TabProcFPR(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		return TRUE;
	}
	if (msg != WM_COMMAND)
	{
		return FALSE;
	}

	WORD notification = HIWORD(wParam);

	if (notification == EN_KILLFOCUS)
	{

		WORD controlID = LOWORD(wParam);
		char regText[9];
		CWindow edit = GetDlgItem(hDlg, controlID);
		edit.GetWindowTextA(regText, 9);
		uint32_t value = strtoul(regText, NULL, 16);
		sprintf(regText, "%08X", value);
		edit.SetWindowTextA(regText);

		CBreakpoints* breakpoints = ((CDebuggerUI*)g_Debugger)->Breakpoints();
		if (g_Reg == NULL || !breakpoints->isDebugging())
		{
			return FALSE;
		}

		int nReg = CDebugCommandsView::MapFPREdit(controlID);

		*(uint32_t*)g_Reg->m_FPR_S[nReg] = value;
	}

	return FALSE;
}

static INT_PTR CALLBACK TabProcPI(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		return TRUE;
	}
	if (msg != WM_COMMAND)
	{
		return FALSE;
	}

	WORD notification = HIWORD(wParam);
	
	if (notification == EN_KILLFOCUS)
	{

		WORD controlID = LOWORD(wParam);
		char regText[9];
		CWindow edit = GetDlgItem(hDlg, controlID);
		edit.GetWindowTextA(regText, 9);
		uint32_t value = strtoul(regText, NULL, 16);
		sprintf(regText, "%08X", value);
		edit.SetWindowTextA(regText);

		CBreakpoints* breakpoints = ((CDebuggerUI*)g_Debugger)->Breakpoints();
		if (g_MMU == NULL || !breakpoints->isDebugging())
		{
			return FALSE;
		}

		int nReg = CDebugCommandsView::MapPIEdit(controlID);

		switch (nReg)
		{
		case 0:  g_Reg->PI_DRAM_ADDR_REG = value; break;
		case 1:  g_Reg->PI_CART_ADDR_REG = value; break;
		case 2:  g_Reg->PI_RD_LEN_REG = value; break;
		case 3:  g_Reg->PI_WR_LEN_REG = value; break;
		case 4:  g_Reg->PI_STATUS_REG = value; break;
		case 5:  g_Reg->PI_BSD_DOM1_LAT_REG = value; break;
		case 6:  g_Reg->PI_BSD_DOM1_PWD_REG = value; break;
		case 7:  g_Reg->PI_BSD_DOM1_PGS_REG = value; break;
		case 8:  g_Reg->PI_BSD_DOM1_RLS_REG = value; break;
		case 9:  g_Reg->PI_BSD_DOM2_LAT_REG = value; break;
		case 10: g_Reg->PI_BSD_DOM2_PWD_REG = value; break;
		case 11: g_Reg->PI_BSD_DOM2_PGS_REG = value; break;
		case 12: g_Reg->PI_BSD_DOM2_RLS_REG = value; break;
		}
	}
	return FALSE;
}

static INT_PTR CALLBACK TabProcCOP0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		return TRUE;
	}
	if (msg != WM_COMMAND)
	{
		return FALSE;
	}

	WORD notification = HIWORD(wParam);

	if (notification != EN_KILLFOCUS)
	{
		return FALSE;
	}
	
	WORD controlID = LOWORD(wParam);
	char regText[9];
	CWindow edit = GetDlgItem(hDlg, controlID);
	edit.GetWindowTextA(regText, 9);
	uint32_t value = strtoul(regText, NULL, 16);
	sprintf(regText, "%08X", value);
	edit.SetWindowTextA(regText);

	CBreakpoints* breakpoints = ((CDebuggerUI*)g_Debugger)->Breakpoints();
	if (g_MMU == NULL || !breakpoints->isDebugging())
	{
		return FALSE;
	}

	int nReg = CDebugCommandsView::MapCOP0Edit(controlID);

	switch (nReg)
	{
	case 0:  g_Reg->INDEX_REGISTER = value; break;
	case 1:  g_Reg->RANDOM_REGISTER = value; break;
	case 2:  g_Reg->ENTRYLO0_REGISTER = value; break;
	case 3:  g_Reg->ENTRYLO1_REGISTER = value; break;
	case 4:  g_Reg->CONTEXT_REGISTER = value; break;
	case 5:  g_Reg->PAGE_MASK_REGISTER = value; break;
	case 6:  g_Reg->WIRED_REGISTER = value; break;
	case 7:  g_Reg->BAD_VADDR_REGISTER = value; break;
	case 8:  g_Reg->COUNT_REGISTER = value; break;
	case 9:  g_Reg->ENTRYHI_REGISTER = value; break;
	case 10: g_Reg->COMPARE_REGISTER = value; break;
	case 11: g_Reg->STATUS_REGISTER = value; break;
	case 12: g_Reg->CAUSE_REGISTER = value; break;
	case 13: g_Reg->EPC_REGISTER = value; break;
	case 14: g_Reg->CONFIG_REGISTER = value; break;
	case 15: g_Reg->TAGLO_REGISTER = value; break;
	case 16: g_Reg->TAGHI_REGISTER = value; break;
	case 17: g_Reg->ERROREPC_REGISTER = value; break;
	case 18: g_Reg->FAKE_CAUSE_REGISTER = value; break;
	}

	return FALSE;
}

CRect CRegisterTabs::GetPageRect()
{
	CWindow parentWin = GetParent();
	CRect pageRect;
	GetWindowRect(&pageRect);
	parentWin.ScreenToClient(&pageRect);
	AdjustRect(FALSE, &pageRect);
	return pageRect;
}

CWindow CRegisterTabs::AddTab(char* caption, int dialogId, DLGPROC dlgProc)
{
	AddItem(caption);
	
	CWindow parentWin = GetParent();
	CWindow tabWin = ::CreateDialog(NULL, MAKEINTRESOURCE(dialogId), parentWin, dlgProc);

	CRect pageRect = GetPageRect();

	::SetParent(tabWin, parentWin);

	::SetWindowPos(
		tabWin,
		m_hWnd,
		pageRect.left,
		pageRect.top,
		pageRect.Width(),
		pageRect.Height(),
		SWP_HIDEWINDOW
	);

	m_TabWindows.push_back(tabWin);
	m_TabIds.push_back(dialogId);
	
	int index = m_TabWindows.size() - 1;

	if (m_TabWindows.size() == 1)
	{
		ShowTab(0);
	}

	return tabWin;

}

void CRegisterTabs::ShowTab(int nPage)
{
	for (int i = 0; i < m_TabWindows.size(); i++)
	{
		::ShowWindow(m_TabWindows[i], SW_HIDE);
	}
	
	CRect pageRect = GetPageRect();

	::SetWindowPos(
		m_TabWindows[nPage],
		m_hWnd,
		pageRect.left,
		pageRect.top,
		pageRect.Width(),
		pageRect.Height(),
		SWP_SHOWWINDOW
	);
}

void CRegisterTabs::RedrawCurrentTab()
{
	int nPage = GetCurSel();
	CRect pageRect = GetPageRect();

	::SetWindowPos(
		m_TabWindows[nPage],
		m_hWnd,
		pageRect.left,
		pageRect.top,
		pageRect.Width(),
		pageRect.Height(),
		SWP_SHOWWINDOW
	);
}

void CDebugCommandsView::ShowPIRegTab()
{
	m_RegisterTabs.SetCurSel(2);
	m_RegisterTabs.ShowTab(2);
}

LRESULT CDebugCommandsView::OnRegisterTabChange(NMHDR* pNMHDR)
{
	int nPage = m_RegisterTabs.GetCurSel();
	m_RegisterTabs.ShowTab(nPage);
	return FALSE;
}

// CEditReg64

uint64_t CEditReg64::ParseValue(char* wordPair)
{
	uint32_t a, b;
	uint64_t ret;
	a = strtoul(wordPair, &wordPair, 16);
	if (*wordPair == ' ')
	{
		wordPair++;
		b = strtoul(wordPair, NULL, 16);
		ret = (uint64_t)a << 32;
		ret |= b;
		return ret;
	}
	return (uint64_t)a;
}

BOOL CEditReg64::Attach(HWND hWndNew)
{
	return SubclassWindow(hWndNew);
}

LRESULT CEditReg64::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CBreakpoints* breakpoints = ((CDebuggerUI*)g_Debugger)->Breakpoints();
	if (!breakpoints->isDebugging())
	{
		goto canceled;
	}

	char charCode = wParam;

	if (!isxdigit(charCode) && charCode != ' ')
	{
		if (!isalnum(charCode))
		{
			goto unhandled;
		}
		goto canceled;
	}

	if (isalpha(charCode) && !isupper(charCode))
	{
		SendMessage(uMsg, toupper(wParam), lParam);
		goto canceled;
	}

	char text[20];
	GetWindowText(text, 20);
	int textLen = strlen(text);

	if (textLen >= 17)
	{
		int selStart, selEnd;
		GetSel(selStart, selEnd);
		if (selEnd - selStart == 0)
		{
			goto canceled;
		}
	}

	if (charCode == ' ' && strchr(text, ' ') != NULL)
	{
		goto canceled;
	}

unhandled:
	bHandled = FALSE;
	return 0;

canceled:
	bHandled = TRUE;
	return 0;
}

uint64_t CEditReg64::GetValue()
{
	char text[20];
	GetWindowText(text, 20);
	return ParseValue(text);
}

LRESULT CEditReg64::OnLostFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SetValue(GetValue()); // clean up
	bHandled = FALSE;
	return 0;
}

void CEditReg64::SetValue(uint32_t h, uint32_t l)
{
	char text[20];
	sprintf(text, "%08X %08X", h, l);
	SetWindowText(text);
}

void CEditReg64::SetValue(uint64_t value)
{
	uint32_t h = (value & 0xFFFFFFFF00000000LL) >> 32;
	uint32_t l = (value & 0x00000000FFFFFFFFLL);
	SetValue(h, l);
}


LRESULT CEditOp::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (m_CommandsWindow == NULL)
	{
		return FALSE;
	}
	return m_CommandsWindow->OnOpKeyDown(uMsg, wParam, lParam, bHandled);
}

void CEditOp::SetCommandsWindow(CDebugCommandsView* commandsWindow)
{
	m_CommandsWindow = commandsWindow;
}

BOOL CEditOp::Attach(HWND hWndNew)
{
	return SubclassWindow(hWndNew);
}