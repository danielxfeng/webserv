#include <gtest/gtest.h>
#include "../../includes/Config.hpp"

TEST(ConfigTest, TestGetGlobalConfig)
{
    EXPECT_EQ(1, 1);
}

TEST(ConfigFromJson, HappyPath_AllBranches)
{
    // Globals + 2 servers: one normal, one CGI.
    // - The normal server requests large per-server limits so clamping via std::min kicks in.
    // - The CGI server omits per-server limits to take global defaults.
    // - index is optional and defaults to "" when omitted.
    const std::string json = R"JSON(
  {
    "max_poll_events": 128,
    "max_poll_timeout": 250,
    "max_connections": 500,
    "global_request_timeout": 2000,
    "max_request_size": 1024,
    "max_headers_size": 4096,
    "servers": [
      {
        "server_name": "app",
        "root": "/var/www/app",
        "index": "index.html",
        "port": 8080,
        "max_request_timeout": 999999,
        "max_request_size": 99999999,
        "max_headers_size": 99999999,
        "is_cgi": false,
        "locations": {
          "/":       ["GET", "POST"],
          "/static": ["GET"]
        }
      },
      {
        "server_name": "cgi1",
        "port": 9000,
        "is_cgi": true,
        "cgi_config": {
          ".py":  "/usr/bin/python3",
          ".php": "/usr/bin/php-cgi"
        }
      }
    ]
  }
  )JSON";

    Config cfg;
    EXPECT_NO_THROW(cfg.fromJson(json));

    auto &g = cfg.getGlobalConfig();
    // globals
    EXPECT_EQ(g.max_poll_events, 128u);
    EXPECT_EQ(g.max_poll_timeout, 250u);
    EXPECT_EQ(g.max_connections, 500u);
    EXPECT_EQ(g.global_request_timeout, 2000u);
    EXPECT_EQ(g.max_request_size, 1024u);
    EXPECT_EQ(g.max_headers_size, 4096u);

    // sanity: both servers present
    ASSERT_TRUE(g.servers.contains("app"));
    ASSERT_TRUE(g.servers.contains("cgi1"));

    // ---- non-CGI server ("app") ----
    const t_server_config &app = g.servers.at("app");
    EXPECT_EQ(app.server_name, "app");
    EXPECT_EQ(app.root, "/var/www/app");
    EXPECT_EQ(app.index, "index.html");
    EXPECT_EQ(app.port, 8080u);
    // Requested bigger than globals → clamped down by std::min
    EXPECT_EQ(app.max_request_timeout, g.global_request_timeout); // 2000
    EXPECT_EQ(app.max_request_size, g.max_request_size);          // 1024
    EXPECT_EQ(app.max_headers_size, g.max_headers_size);          // 4096
    EXPECT_FALSE(app.is_cgi);

    // locations parsed and methods converted (no CGI/UNKNOWN)
    ASSERT_TRUE(app.locations.contains("/"));
    ASSERT_TRUE(app.locations.contains("/static"));
    const auto &root_methods = app.locations.at("/");
    const auto &static_methods = app.locations.at("/static");
    ASSERT_EQ(root_methods.size(), 2u);
    ASSERT_EQ(static_methods.size(), 1u);
    EXPECT_EQ(root_methods[0], GET);
    EXPECT_EQ(root_methods[1], POST);
    EXPECT_EQ(static_methods[0], GET);

    // ---- CGI server ("cgi1") ----
    const t_server_config &cgi = g.servers.at("cgi1");
    EXPECT_EQ(cgi.server_name, "cgi1");
    // index omitted → default ""
    EXPECT_EQ(cgi.index, "");
    EXPECT_EQ(cgi.port, 9000u);
    EXPECT_TRUE(cgi.is_cgi);

    // per-server limits omitted → inherit via defaults (then clamped, but equals globals)
    EXPECT_EQ(cgi.max_request_timeout, g.global_request_timeout);
    EXPECT_EQ(cgi.max_request_size, g.max_request_size);
    EXPECT_EQ(cgi.max_headers_size, g.max_headers_size);

    // cgi_config parsed
    ASSERT_EQ(cgi.cgi_paths.size(), 2u);
    EXPECT_EQ(cgi.cgi_paths.at(".py"), "/usr/bin/python3");
    EXPECT_EQ(cgi.cgi_paths.at(".php"), "/usr/bin/php-cgi");
}