#include "defaulttrackerwindow.h"
#include "../uilib/hbox.h"
#include "../core/assets.h"

namespace Ui {

DefaultTrackerWindow::DefaultTrackerWindow(const char* title, SDL_Surface* icon, const Position& pos, const Size& size)
    : TrackerWindow(title, icon, pos, size)
{
    auto hbox = new HBox(0,0,_size.width,32);
    hbox->setBackground({0,0,0,255});
    hbox->setMargin(2);
    hbox->setSpacing(2);
    hbox->setGrow(1,1);
    _menu = hbox;
    addChild(_menu);
    
    _btnLoad = new ImageButton(0,0,32-4,32-4, asset("load.png").c_str());
    _btnLoad->setDarkenGreyscale(false);
    hbox->addChild(_btnLoad);
    _btnLoad->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_LOAD);
    }};
    
    _btnReload = new ImageButton(0,0,32-4,32-4, asset("reload.png").c_str());
    _btnReload->setVisible(false);
    hbox->addChild(_btnReload);
    _btnReload->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_RELOAD);
    }};
    
#ifndef __EMSCRIPTEN__ // no multi-window support (yet)
    _btnBroadcast = new ImageButton(0,0,32-4,32-4, asset("broadcast.png").c_str());
    _btnBroadcast->setVisible(false);
    hbox->addChild(_btnBroadcast);
    _btnBroadcast->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_BROADCAST);
    }};
#endif
    
    _btnPackSettings = new ImageButton(32-4,0,32-4,32-4, asset("settings.png").c_str());
    _btnPackSettings->setVisible(false);
    hbox->addChild(_btnPackSettings);
    _btnPackSettings->onClick += { this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_PACK_SETTINGS);
    }};
    
    _lblAutoTracker = new Label(0,0,32-4,32-4,_font, "AT");
    _lblAutoTracker->setTextColor({0,0,0});
    hbox->addChild(_lblAutoTracker);
    _lblAutoTracker->onClick += {this, [this](void*, int x, int y, int button) {
        onMenuPressed.emit(this, MENU_TOGGLE_AUTOTRACKER);
    }};
    
    _lblTooltip = new Label(0,0,0,0,_font,"");
    _lblTooltip->setGrow(1,1);
    _lblTooltip->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::MIDDLE);
    hbox->addChild(_lblTooltip);
    
    _loadPackWidget = new LoadPackWidget(0,0,0,0,_fontStore);
    _loadPackWidget->setPosition({0,0});
    _loadPackWidget->setSize(_size);
    _loadPackWidget->setBackground({0,0,0});
    _loadPackWidget->setVisible(false);
    addChild(_loadPackWidget);
    
    _loadPackWidget->onPackSelected += {this,[this](void *s, const std::string& pack, const std::string& variant) {
        onPackSelected.emit(this,pack,variant);
    }};
    
    for (const auto& pair : {
            std::pair{_btnLoad, "Load Pack"},
            std::pair{_btnReload, "Reload Pack"},
            std::pair{_btnBroadcast, "Open Broadcast View"},
            std::pair{_btnPackSettings, "Open Pack Settings"}
    }) {
        if (!pair.first) continue;
        pair.first->setDarkenGreyscale(false);
        pair.first->setEnabled(false); // make greyscale
        const char* text = pair.second; // pointer to program memory
        pair.first->onMouseEnter += {this, [this,text](void *s, int x, int y, unsigned buttons)
        {
            ImageButton* btn = (ImageButton*)s;
            btn->setEnabled(true); // disable greyscale
            _lblTooltip->setText(text);
        }};
        pair.first->onMouseLeave += {this, [this,text](void *s)
        {
            ImageButton* btn = (ImageButton*)s;
            btn->setEnabled(false); // enable greyscale
            _lblTooltip->setText("");
        }};
    }
    _lblAutoTracker->onMouseEnter += {this, [this](void *s, int x, int y, unsigned buttons)
    {
        const char* state = _autoTrackerState == AutoTracker::State::Disconnected ? "Offline" :
                            _autoTrackerState == AutoTracker::State::BridgeConnected ? "No Game/Console" :
                            _autoTrackerState == AutoTracker::State::ConsoleConnected ? "Online" :
                            _autoTrackerState == AutoTracker::State::Disabled ? "Disabled" :
                            _autoTrackerState == AutoTracker::State::Unavailable ? nullptr :
                                "Unknown";
        if (state)
            _lblTooltip->setText(std::string("Auto Tracker: ") + state);
    }};
    _lblAutoTracker->onMouseLeave += {this, [this](void *s)
    {
        _lblTooltip->setText("");
    }};
}

DefaultTrackerWindow::~DefaultTrackerWindow()
{
    _btnLoad = nullptr;
    _btnReload = nullptr;
    _btnBroadcast = nullptr;
    _btnPackSettings = nullptr;
    _lblAutoTracker = nullptr;
}

void DefaultTrackerWindow::setTracker(Tracker* tracker, const std::string& layout)
{
    _lblAutoTracker->setTextColor({0,0,0});
    TrackerWindow::setTracker(tracker, layout);
    if (tracker) {
        tracker->onLayoutChanged += {this, [this,tracker](void *s, const std::string& layout) {
            if (_btnBroadcast) _btnBroadcast->setVisible(tracker->hasLayout("tracker_broadcast"));
            if (_btnPackSettings) _btnPackSettings->setVisible(tracker->hasLayout("settings_popup"));
            if (!tracker->hasLayout("tracker_default")) {
                // TODO: get preferred orientation from settings
                if (tracker->hasLayout("tracker_horizontal")) {
                    tracker->onLayoutChanged -= this;
                    setTracker(tracker, "tracker_horizontal");
                }
                else if (tracker->hasLayout("tracker_vertical")) {
                    tracker->onLayoutChanged -= this;
                    setTracker(tracker, "tracker_vertical");
                }
            }
        }};
        _view->onItemHover += {this, [this,tracker](void *s, const std::string& itemid) {
            if (itemid.empty()) _lblTooltip->setText("");
            else {
                const auto& item = tracker->getItemById(itemid);
                _lblTooltip->setText(item.getName()); 
            }
        }};
        if (_btnReload) _btnReload->setVisible(true);
    } else {
        if (_btnBroadcast) _btnBroadcast->setVisible(false);
        if (_btnPackSettings) _btnPackSettings->setVisible(false);
        if (_btnReload) _btnReload->setVisible(false);
        
    }
    raiseChild(_loadPackWidget);
}

void DefaultTrackerWindow::setAutoTrackerState(AutoTracker::State state)
{
    _autoTrackerState = state;
    if (!_lblAutoTracker) return;
    if (state == AutoTracker::State::Disconnected) {
        _lblAutoTracker->setTextColor({255,0,0});
    } else if (state == AutoTracker::State::BridgeConnected) {
        _lblAutoTracker->setTextColor({255,255,0});
    } else if (state == AutoTracker::State::ConsoleConnected) {
        _lblAutoTracker->setTextColor({0,255,0});
    } else if (state == AutoTracker::State::Disabled) {
        _lblAutoTracker->setTextColor({128,128,128});
    } else if (state == AutoTracker::State::Unavailable) {
        _lblAutoTracker->setTextColor({0,0,0}); // hidden
    }

    if (isHover(_lblAutoTracker)) {
        // update tool tip if current tool tip is AT state
        _lblAutoTracker->onMouseEnter.emit(_lblAutoTracker, 0, 0, 0);
    }
}

void DefaultTrackerWindow::setSize(Size size) {
    TrackerWindow::setSize(size);
    _loadPackWidget->setSize(size);
}

void DefaultTrackerWindow::showOpen() {
    _loadPackWidget->update();
    _loadPackWidget->setVisible(true);
}
void DefaultTrackerWindow::hideOpen() {
    _loadPackWidget->setVisible(false);
}

} // namespace

