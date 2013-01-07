#ifndef OP_H
#define OP_H

#include "include\cef_base.h"
#include "include\cef_runnable.h"
#include "include\cef_browser.h"

class Op: public virtual CefBase {

	const CefRefPtr<CefBrowser> m_browser;

public:

	Op(CefRefPtr<CefBrowser> browser) : m_browser(browser) {}

	static void RunOp(CefRefPtr<Op>);

	virtual CefRefPtr<CefProcessMessage> Do() const = 0;

	void OnEnd(CefRefPtr<CefProcessMessage> response) const {
		m_browser->SendProcessMessage(PID_RENDERER, response);
	}		
};

#endif