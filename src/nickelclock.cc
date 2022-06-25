#include <cstddef>
#include <cstdlib>

#include <Qt>
#include <QWidget>
#include <QHBoxLayout>
#include <QFont>
#include <QFile>
#include <QLabel>
#include <QVariant>

#include <NickelHook.h>

const char nc_qt_property[] = "NickelClock";
const char nc_widget_name[] = "ncTimeLabel";

typedef QWidget ReadingView;
typedef QWidget ReadingFooter;
typedef QLabel TimeLabel;
typedef QLabel TouchLabel;

enum class TimePos {Left, Right};

void (*ReadingView__UpdateProgressHeader)(ReadingView *_this, QString *p1, QString *p2);
TimeLabel *(*TimeLabel__TimeLabel)(TimeLabel *_this, QWidget *parent);

static struct nh_info NickelClock = {
    .name           = "NickelClock",
    .desc           = "Set an always displayed clock when reading",
    .uninstall_flag = "/mnt/onboard/.adds/nc_uninstall",
    .uninstall_xflag = nullptr,
    .failsafe_delay = 10
};

static struct nh_hook NickelClockHook[] = {
    {
        .sym     = "_ZN11ReadingView20updateProgressHeaderERK7QStringS2_", 
        .sym_new = "_nc_set_header_clock",
        .lib     = "libnickel.so.1.0.0",
        .out     = nh_symoutptr(ReadingView__UpdateProgressHeader),
        .desc    = "footer progress update"
    },
    {0},
};

static struct nh_dlsym NickelClockDlsym[] = {
    {
        .name    = "_ZN9TimeLabelC1EP7QWidget",
        .out     = nh_symoutptr(TimeLabel__TimeLabel),
        .desc    = "TimeLabel::TimeLabel()"
    },
    {0},
};

NickelHook(
    .init  = nullptr,
    .info  = &NickelClock,
    .hook  = NickelClockHook,
    .dlsym = NickelClockDlsym,
)

// Sets the TimeLabel style to the same as the footer text style
static QString get_time_style() {
    QFile rfStyleFile(":/qss/ReadingFooter.qss");
    if (rfStyleFile.open(QIODevice::ReadOnly)) {
        QString style = rfStyleFile.readAll();
        style.replace("#caption", QString("#%1").arg(nc_widget_name));
        return style;
    }
    return "";
}

static void add_time_to_footer(ReadingFooter *rf, TimePos position) {
    QLayout *l = nullptr;
    if (rf && !rf->property(nc_qt_property).isValid() && (l = rf->layout())) {
        nh_log("ReadingView header layout found");
        rf->setProperty(nc_qt_property, true);
        QHBoxLayout *hl = qobject_cast<QHBoxLayout*>(l);
        if (hl) {
            nh_log("Adding TimeLabel widget to ReadingView header");
            hl->setStretch(0, 2);

            TimeLabel *tl = (TimeLabel*) ::operator new (128); // Actual size 88 bytes
            TimeLabel__TimeLabel(tl, nullptr);
            tl->setObjectName(nc_widget_name);
            tl->setStyleSheet(get_time_style());

            if (position == TimePos::Left) {
                hl->insertWidget(0, tl, 1, Qt::AlignLeft);
                hl->addStretch(1);
            } else {
                hl->insertStretch(0, 1);
                hl->addWidget(tl, 1, Qt::AlignRight);
            }
        }
    }
}

extern "C" __attribute__((visibility("default"))) void _nc_set_header_clock(ReadingView *_this, QString *p1, QString *p2) {
    // Find header
    ReadingFooter *rf = _this->findChild<ReadingFooter*>("header");
    add_time_to_footer(rf, TimePos::Right);
    ReadingView__UpdateProgressHeader(_this, p1, p2);
}
