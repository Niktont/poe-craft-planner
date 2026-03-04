#include "WebPage.h"

namespace planner {

WebPage::WebPage(QWebEngineProfile* profile, QObject* parent)
    : QWebEnginePage{profile, parent}
{
    connect(this,
            &QWebEnginePage::selectClientCertificate,
            this,
            &WebPage::handleSelectClientCertificate);
    connect(this, &QWebEnginePage::certificateError, this, &WebPage::handleCertificateError);
}

void WebPage::handleCertificateError(QWebEngineCertificateError error)
{
    error.rejectCertificate();
}

void WebPage::handleSelectClientCertificate(QWebEngineClientCertificateSelection selection)
{
    selection.select(selection.certificates().at(0));
}

} // namespace planner
