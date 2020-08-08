#include "pch.h"

#include "MainPage.h"

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "MainPage.g.cpp"

using ReactTestApp::JSBundleSource;
using winrt::Microsoft::ReactNative::IJSValueWriter;
using winrt::Windows::ApplicationModel::Core::CoreApplication;
using winrt::Windows::ApplicationModel::Core::CoreApplicationViewTitleBar;
using winrt::Windows::Foundation::IAsyncAction;
using winrt::Windows::UI::Colors;
using winrt::Windows::UI::ViewManagement::ApplicationView;
using winrt::Windows::UI::Xaml::RoutedEventArgs;
using winrt::Windows::UI::Xaml::RoutedEventHandler;
using winrt::Windows::UI::Xaml::Window;
using winrt::Windows::UI::Xaml::Controls::MenuFlyout;
using winrt::Windows::UI::Xaml::Controls::MenuFlyoutItem;
using winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs;

namespace
{
    void WritePropertyValue(std::any const &propertyValue, IJSValueWriter const &writer)
    {
        if (propertyValue.type() == typeid(boolean)) {
            writer.WriteBoolean(std::any_cast<boolean>(propertyValue));
        } else if (propertyValue.type() == typeid(std::int64_t)) {
            writer.WriteInt64(std::any_cast<std::int64_t>(propertyValue));
        } else if (propertyValue.type() == typeid(std::uint64_t)) {
            writer.WriteInt64(std::any_cast<std::uint64_t>(propertyValue));
        } else if (propertyValue.type() == typeid(double)) {
            writer.WriteDouble(std::any_cast<double>(propertyValue));
        } else if (propertyValue.type() == typeid(std::nullopt)) {
            writer.WriteNull();
        } else if (propertyValue.type() == typeid(std::string)) {
            writer.WriteString(winrt::to_hstring(std::any_cast<std::string>(propertyValue)));
        } else if (propertyValue.type() == typeid(std::vector<std::any>)) {
            writer.WriteArrayBegin();
            for (auto &&e : std::any_cast<std::vector<std::any>>(propertyValue)) {
                WritePropertyValue(e, writer);
            }
            writer.WriteArrayEnd();
        } else if (propertyValue.type() == typeid(std::map<std::string, std::any>)) {
            writer.WriteObjectBegin();
            for (auto &&e : std::any_cast<std::map<std::string, std::any>>(propertyValue)) {
                writer.WritePropertyName(winrt::to_hstring(e.first));
                WritePropertyValue(e.second, writer);
            }
            writer.WriteObjectEnd();
        } else {
            assert(false);
        }
    }
}  // namespace

namespace winrt::ReactTestApp::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();

        SetUpTitleBar();

        auto menuItems = ReactMenuBarItem().Items();
        std::optional<::ReactTestApp::Manifest> manifest = ::ReactTestApp::GetManifest();
        if (!manifest.has_value()) {
            MenuFlyoutItem newMenuItem;
            newMenuItem.Text(L"Couldn't parse app.json");
            newMenuItem.IsEnabled(false);
            menuItems.Append(newMenuItem);
        } else {
            ReactRootView().ReactNativeHost(reactInstance_.ReactHost());

            // If only one component is present load it automatically
            auto &components = manifest.value().components;
            if (components.size() == 1) {
                LoadReactComponent(components.at(0));
            } else {
                AppTitle().Text(to_hstring(manifest.value().displayName));
            }

            for (auto &&c : components) {
                MenuFlyoutItem newMenuItem;
                newMenuItem.Text(winrt::to_hstring(c.displayName.value_or(c.appKey)));
                newMenuItem.Click(
                    [this, component = std::move(c)](IInspectable const &, RoutedEventArgs) {
                        LoadReactComponent(component);
                    });
                menuItems.Append(newMenuItem);
            }
        }
    }

    void MainPage::LoadFromDevServer(IInspectable const &, RoutedEventArgs)
    {
        reactInstance_.LoadJSBundleFrom(JSBundleSource::DevServer);
    }

    void MainPage::LoadFromJSBundle(IInspectable const &, RoutedEventArgs)
    {
        reactInstance_.LoadJSBundleFrom(JSBundleSource::Embedded);
    }

    IAsyncAction MainPage::OnNavigatedTo(NavigationEventArgs const &e)
    {
        Base::OnNavigatedTo(e);

        bool devServerIsRunning = co_await ::ReactTestApp::IsDevServerRunning();
        if (devServerIsRunning) {
            reactInstance_.LoadJSBundleFrom(JSBundleSource::DevServer);
        } else {
            reactInstance_.LoadJSBundleFrom(JSBundleSource::Embedded);
        }
    }

    void MainPage::LoadReactComponent(::ReactTestApp::Component const &component)
    {
        AppTitle().Text(to_hstring(component.displayName.value_or(component.appKey)));

        ReactRootView().ComponentName(to_hstring(component.appKey));
        ReactRootView().InitialProps(
            [&initialProps = component.initialProperties](IJSValueWriter const &writer) {
                if (initialProps.has_value()) {
                    writer.WriteObjectBegin();
                    for (auto &&property : initialProps.value()) {
                        auto &value = property.second;
                        writer.WritePropertyName(to_hstring(property.first));
                        WritePropertyValue(value, writer);
                    }
                    writer.WriteObjectEnd();
                }
            });
    }

    void MainPage::SetUpTitleBar()
    {
        auto coreTitleBar = CoreApplication::GetCurrentView().TitleBar();
        coreTitleBar.LayoutMetricsChanged({this, &MainPage::OnCoreTitleBarLayoutMetricsChanged});
        coreTitleBar.ExtendViewIntoTitleBar(true);

        // Set close, minimize and maximize icons background to transparent
        auto viewTitleBar = ApplicationView::GetForCurrentView().TitleBar();
        viewTitleBar.ButtonBackgroundColor(Colors::Transparent());
        viewTitleBar.ButtonInactiveBackgroundColor(Colors::Transparent());

        Window::Current().SetTitleBar(AppTitleBar());
    }

    // Adjust height of custom title bar to match close, minimize and maximize icons
    void MainPage::OnCoreTitleBarLayoutMetricsChanged(CoreApplicationViewTitleBar const &sender,
                                                      IInspectable const &)
    {
        AppTitleBar().Height(sender.Height());
        AppMenuBar().Height(sender.Height());
    }
}  // namespace winrt::ReactTestApp::implementation