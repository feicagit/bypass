#ifndef NET_CURL_HTTP_TRANS_THREAD_H_
#define NET_CURL_HTTP_TRANS_THREAD_H_

#include <memory>
#include "base/thread/framework_thread.h"
#include "base/memory/singleton.h"
#include "net/curl/curl_network_session_manager.h"
#include "net/http/curl_http_request.h"
#include "net/base/proxy.h"

namespace net
{

typedef std::shared_ptr<CurlHttpRequest> HttpRequestSharedPtr;

class NET_EXPORT HttpTransThread : public nbase::FrameworkThread
{
public:
	SINGLETON_DEFINE(HttpTransThread);
	HttpTransThread();
	
	void Start(net::Proxy proxy);
	void Stop();

	int PostRequest(const HttpRequestSharedPtr &request);
	void RemoveRequest(int http_request_id);

private:
	void Init() override;
	void Cleanup() override;
	static void DoHttpRequestTask(const HttpRequestSharedPtr &request);
	static void DoRemoveHttpRequestTask(net::CurlHttpRequest* request);
	void AddSession(const HttpRequestSharedPtr& request);
	void RemoveSession(net::CurlHttpRequest* request);

private:
	std::unique_ptr<net::CurlNetworkSessionManager> manager_; // Http会话管理器
	net::Proxy proxy_;
};

}


#endif //NET_CURL_HTTP_TRANS_THREAD_H_