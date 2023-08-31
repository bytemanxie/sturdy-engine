// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "CWatchDialog.h"
#include "afxdialogex.h"
#include "RemoteClientDlg.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialogEx)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjWidth = -1;
	m_nObjHeight = -1;

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialogEx)
//	ON_WM_TIMER()
ON_WM_TIMER()
ON_WM_LBUTTONDBLCLK()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
ON_WM_RBUTTONDBLCLK()
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()
ON_WM_MOUSEMOVE()
ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	CRect clientRect;
	if(isScreen) m_picture.ScreenToClient(&point);//全局坐标到客户区域坐标
	else
	{
		ClientToScreen(&point);//转换为相对屏幕左上角的坐标（屏幕的绝对坐标）
		m_picture.ScreenToClient(&point);//转换为客户区域坐标（相对picture控件左上角坐标）
	}
	m_picture.GetWindowRect(clientRect);
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = m_nObjWidth, height = m_nObjHeight;
	int x = point.x * width / width0;
	int y = point.y * height / height0;

	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false;
	SetTimer(0, 50, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	 //TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0)
	{
		CClientController* pParent = CClientController::getInstance();
		if (m_isFull)
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			m_nObjWidth = m_image.GetWidth();
			
			m_nObjHeight = m_image.GetHeight();
			
			m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0,
				rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			m_image.Destroy();
			m_isFull = false;
			TRACE("更新图片成功 %d %d %08X\r\n", m_nObjWidth, m_nObjHeight, 
				(HBITMAP)m_image);
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		//坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		////封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 2;
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
		
	}
	CDialogEx::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//双击
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
	}
	
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 3;
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		////封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 1;
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
	}
	

	CDialogEx::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 2;//按下
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
	}
	

	CDialogEx::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 3;//弹起
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
	}
	

	CDialogEx::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值  
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;
		event.nAction = 0;
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
	}

	CDialogEx::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1))
	{
		CPoint point;
		GetCursorPos(&point);
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;
		event.nAction = 0;
		CClientController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof event);
	}
	

}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialogEx::OnOK();
}


void CWatchDialog::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->SendCommandPacket(7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->SendCommandPacket(8);
}
