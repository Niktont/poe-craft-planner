#include "WebViewDialog.h"
#include "Settings.h"
#include "WebPage.h"
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineProfileBuilder>
#include <QWebEngineSettings>
#include <QtWebEngineWidgets/QWebEngineView>

using namespace Qt::Literals;

namespace planner {

WebViewDialog::WebViewDialog(const QString& user_agent, QWidget* parent)
    : QDialog{parent}
{
    auto layout = new QVBoxLayout{};
    setLayout(layout);

    setWindowTitle(tr("Web Page"));
    auto settings = Settings::get();
    auto geometry = settings.value(Settings::windows_web_view_dialog_geometry);
    if (!geometry.isValid())
        resize(900, 800);
    else
        restoreGeometry(geometry.toByteArray());

    QWebEngineProfileBuilder profileBuilder;
    web_profile.reset(profileBuilder.createProfile(
        u"profile."_s + QLatin1StringView(qWebEngineChromiumVersion())));
    web_profile->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    web_profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    web_profile->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    web_profile->setHttpUserAgent(user_agent);

    web_view = new QWebEngineView{this};
    web_view->setPage(new WebPage{web_profile.get(), web_view});
    web_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(web_view->page(), &QWebEnginePage::titleChanged, this, &QDialog::setWindowTitle);

    url_edit = new QLineEdit{this};
    connect(url_edit, &QLineEdit::returnPressed, this, [this] {
        web_view->setUrl(QUrl::fromUserInput(url_edit->text()));
    });
    connect(web_view, &QWebEngineView::urlChanged, this, [this](const QUrl& url) {
        url_edit->setText(url.toDisplayString());
    });

    auto button = new QPushButton{tr("Clear cookies"), this};
    connect(button, &QPushButton::clicked, this, [this] {
        web_profile->cookieStore()->deleteAllCookies();
    });

    auto url_layout = new QHBoxLayout{};
    url_layout->addWidget(url_edit, 1);
    url_layout->addWidget(button);
    layout->addLayout(url_layout);

    layout->addWidget(web_view);

    button->setAutoDefault(false);

    web_view->load({""});
}

void WebViewDialog::cleanup()
{
    web_view->page()->deleteLater();
}

} // namespace planner
