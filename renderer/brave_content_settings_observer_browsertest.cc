/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "net/dns/mock_host_resolver.h"

const char kIframeID[] = "test";
const char kScript[] =
  "var canvas = document.createElement('canvas');"
  "var ctx = canvas.getContext('2d');"
  "ctx.rect(10, 10, 100, 100);"
  "ctx.stroke();"
  "domAutomationController.send(ctx.isPointInPath(10, 10));";

class BraveContentSettingsObserverBrowserTest : public InProcessBrowserTest {
  public:
    void SetUpOnMainThread() override {
      InProcessBrowserTest::SetUpOnMainThread();
      host_resolver()->AddRule("*", "127.0.0.1");
      content::SetupCrossSiteRedirector(embedded_test_server());

      brave::RegisterPathProvider();
      base::FilePath test_data_dir;
      PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
      embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

      ASSERT_TRUE(embedded_test_server()->Start());

      url_ = embedded_test_server()->GetURL("a.com", "/iframe.html");
      iframe_url_ = embedded_test_server()->GetURL("b.com", "/simple.html");
      primary_url_ = GURL("http://a.com/*");
    }

    const GURL& url() { return url_; }
    const GURL& iframe_url() { return iframe_url_; }
    const GURL& primary_url() { return primary_url_; }

  private:
    GURL url_;
    GURL iframe_url_;
    GURL primary_url_;
};

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, BlockThirdPartyFPByDefault) {
  ui_test_utils::NavigateToURL(browser(), url());
  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(contents->GetAllFrames().size(), (unsigned)2);

  content::RenderFrameHost* main_frame = contents->GetMainFrame();
  content::RenderFrameHost* child_frame =
    ChildFrameAt(contents->GetMainFrame(), 0);
  EXPECT_EQ(main_frame->GetLastCommittedURL(), url());

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, kScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents, kIframeID, iframe_url()));
  EXPECT_EQ(child_frame->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame, kScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, BlockFP) {
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingDefaultScope(primary_url(), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, "fingerprinting", CONTENT_SETTING_BLOCK);

  ui_test_utils::NavigateToURL(browser(), url());
  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(contents->GetAllFrames().size(), (unsigned)2);

  content::RenderFrameHost* main_frame = contents->GetMainFrame();
  content::RenderFrameHost* child_frame =
    ChildFrameAt(contents->GetMainFrame(), 0);
  EXPECT_EQ(main_frame->GetLastCommittedURL(), url());

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, kScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents, kIframeID, iframe_url()));
  EXPECT_EQ(child_frame->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame, kScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, AllowFP) {
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingDefaultScope(primary_url(), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, "fingerprinting", CONTENT_SETTING_ALLOW);

  ui_test_utils::NavigateToURL(browser(), url());
  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(contents->GetAllFrames().size(), (unsigned)2);

  content::RenderFrameHost* main_frame = contents->GetMainFrame();
  content::RenderFrameHost* child_frame =
    ChildFrameAt(contents->GetMainFrame(), 0);
  EXPECT_EQ(main_frame->GetLastCommittedURL(), url());

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, kScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents, kIframeID, iframe_url()));
  EXPECT_EQ(child_frame->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame, kScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);
}

IN_PROC_BROWSER_TEST_F(BraveContentSettingsObserverBrowserTest, BlockThirdPartyFP) {
  HostContentSettingsMap* content_settings =
      HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  content_settings->SetContentSettingDefaultScope(primary_url(), GURL(),
      CONTENT_SETTINGS_TYPE_PLUGINS, "fingerprinting",
      CONTENT_SETTING_BLOCK_THIRD_PARTY);

  ui_test_utils::NavigateToURL(browser(), url());
  content::WebContents* contents =
    browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(contents->GetAllFrames().size(), (unsigned)2);

  content::RenderFrameHost* main_frame = contents->GetMainFrame();
  content::RenderFrameHost* child_frame =
    ChildFrameAt(contents->GetMainFrame(), 0);
  EXPECT_EQ(main_frame->GetLastCommittedURL(), url());

  bool isPointInPath;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(contents, kScript, &isPointInPath));
  EXPECT_TRUE(isPointInPath);

  EXPECT_TRUE(NavigateIframeToURL(contents, kIframeID, iframe_url()));
  EXPECT_EQ(child_frame->GetLastCommittedURL(), iframe_url());
  EXPECT_TRUE(ExecuteScriptAndExtractBool(
      child_frame, kScript, &isPointInPath));
  EXPECT_FALSE(isPointInPath);
}
