#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebEngineCertificateError>
#include <QWebEnginePage>

namespace planner {

class WebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit WebPage(QWebEngineProfile* profile, QObject* parent = nullptr);

private slots:
    void handleCertificateError(QWebEngineCertificateError error);
    void handleSelectClientCertificate(QWebEngineClientCertificateSelection clientCertSelection);
};

} // namespace planner

#endif // WEBPAGE_H
