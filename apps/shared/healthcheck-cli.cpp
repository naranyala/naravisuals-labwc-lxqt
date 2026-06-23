#include <QCoreApplication>
#include <QTextStream>
#include <QStringList>
#include <QMap>
#include "healthcheck.h"

void printUsage() {
    QTextStream out(stdout);
    out << "Naravisuals Health Check CLI\n\n"
        << "Usage:\n"
        << "  healthcheck [options] [category...]\n\n"
        << "Categories:\n"
        << "  all        Run all checks (default)\n"
        << "  env        Environment variables\n"
        << "  services   Systemd services\n"
        << "  wm         Window manager config\n"
        << "  theme      Theme consistency\n"
        << "  network    Network connectivity\n"
        << "  audio      Audio subsystem\n"
        << "  font       Font rendering (alias: fonts)\n\n"
        << "Options:\n"
        << "  --fix      Auto-fix issues where possible\n"
        << "  --json     Machine-readable output\n"
        << "  --quiet    Only show issues (no headers)\n"
        << "  --list     List available categories\n"
        << "  --help     Show this help\n\n"
        << "Examples:\n"
        << "  healthcheck                  # Run all checks\n"
        << "  healthcheck env wm           # Check env vars and window manager\n"
        << "  healthcheck --fix all        # Run all checks and auto-fix issues\n"
        << "  healthcheck --json env       # JSON output for env checks\n"
        << "  healthcheck --quiet network  # Only show network issues\n";
}

void printCategoryList() {
    QTextStream out(stdout);
    out << "Available categories:\n"
        << "  env        Environment variables (XDG, Qt, GTK, cursor, font rendering)\n"
        << "  services   Systemd user services (panel, notifications, PolicyKit)\n"
        << "  wm         Window manager config (labwc rc.xml, keybinds, theme)\n"
        << "  theme      Theme consistency (GTK3/GTK4 match, icons, cursors)\n"
        << "  network    Network connectivity (internet, DNS, NetworkManager)\n"
        << "  audio      Audio subsystem (PipeWire, PulseAudio, WirePlumber)\n"
        << "  font       Font rendering (fontconfig, env vars, fonts, cache)\n"
        << "  all        All of the above\n";
}

void printIssueText(const HealthIssue &issue, bool quiet) {
    QTextStream out(stdout);
    QString status = issue.detected ? "FAIL" : " OK ";
    QString prefix = quiet ? "" : "  ";

    if (issue.detected) {
        out << prefix << "\033[31m[" << status << "]\033[0m " << issue.title << "\n";
        out << prefix << "        " << issue.description << "\n";
        if (issue.autoFixable && !issue.fixDescription.isEmpty()) {
            out << prefix << "        Fix: " << issue.fixDescription << "\n";
        }
    } else {
        out << prefix << "\033[32m[" << status << "]\033[0m " << issue.title << "\n";
        if (!quiet) {
            out << prefix << "        " << issue.description << "\n";
        }
    }
}

void printIssueJson(const HealthIssue &issue, bool &first) {
    QTextStream out(stdout);
    if (!first) out << ",";
    first = false;
    QString desc = issue.description;
    desc.replace("\"", "\\\"");
    QString fix = issue.fixDescription;
    fix.replace("\"", "\\\"");
    out << "\n  {"
        << "\n    \"id\": \"" << issue.id << "\","
        << "\n    \"category\": \"" << issue.category << "\","
        << "\n    \"title\": \"" << issue.title << "\","
        << "\n    \"description\": \"" << desc << "\","
        << "\n    \"detected\": " << (issue.detected ? "true" : "false") << ","
        << "\n    \"autoFixable\": " << (issue.autoFixable ? "true" : "false");
    if (!issue.fixDescription.isEmpty()) {
        out << ","
            << "\n    \"fixDescription\": \"" << fix << "\"";
    }
    out << "\n  }";
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    QStringList args = app.arguments().mid(1);
    bool doFix = false;
    bool jsonMode = false;
    bool quietMode = false;
    bool listMode = false;
    QStringList categories;

    for (const QString &arg : args) {
        if (arg == "--fix") doFix = true;
        else if (arg == "--json") jsonMode = true;
        else if (arg == "--quiet") quietMode = true;
        else if (arg == "--list") listMode = true;
        else if (arg == "--help" || arg == "-h") { printUsage(); return 0; }
        else if (arg.startsWith("-")) { err << "Unknown option: " << arg << "\n"; return 1; }
        else categories.append(arg);
    }

    if (listMode) { printCategoryList(); return 0; }
    if (categories.isEmpty()) categories << "all";

    // Map category names to checkers
    QMap<QString, std::function<HealthChecker*(QObject*)>> checkerMap;
    checkerMap["env"] = [](QObject *p) -> HealthChecker* { return new EnvHealthChecker(p); };
    checkerMap["services"] = [](QObject *p) -> HealthChecker* { return new ServiceHealthChecker(p); };
    checkerMap["wm"] = [](QObject *p) -> HealthChecker* { return new WmHealthChecker(p); };
    checkerMap["theme"] = [](QObject *p) -> HealthChecker* { return new ThemeHealthChecker(p); };
    checkerMap["network"] = [](QObject *p) -> HealthChecker* { return new NetworkHealthChecker(p); };
    checkerMap["audio"] = [](QObject *p) -> HealthChecker* { return new AudioHealthChecker(p); };
    checkerMap["font"] = [](QObject *p) -> HealthChecker* { return new HealthChecker(p); };
    checkerMap["fonts"] = checkerMap["font"];

    // Expand "all"
    if (categories.contains("all")) {
        categories = {"env", "services", "wm", "theme", "network", "audio", "font"};
    }

    // Validate categories
    for (const QString &cat : categories) {
        if (!checkerMap.contains(cat)) {
            err << "Unknown category: " << cat << "\n";
            err << "Run with --list to see available categories.\n";
            return 1;
        }
    }

    // Run checks
    QList<HealthIssue> allIssues;
    for (const QString &cat : categories) {
        HealthChecker *checker = checkerMap[cat](&app);
        QList<HealthIssue> issues = checker->runCheck();
        allIssues.append(issues);
        delete checker;
    }

    // Output results
    if (jsonMode) {
        out << "{\n  \"issues\": [";
        bool first = true;
        for (const auto &issue : allIssues) {
            printIssueJson(issue, first);
        }
        out << "\n  ],\n  \"summary\": {"
            << "\n    \"total\": " << allIssues.size() << ","
            << "\n    \"issues\": " << [&allIssues]() {
                int c = 0;
                for (const auto &i : allIssues) if (i.detected) c++;
                return c;
            }() << ","
            << "\n    \"ok\": " << [&allIssues]() {
                int c = 0;
                for (const auto &i : allIssues) if (!i.detected) c++;
                return c;
            }()
            << "\n  }\n}\n";
    } else {
        if (!quietMode) {
            out << "\n  Naravisuals Health Check\n";
            out << "  ════════════════════════\n\n";
        }

        QString lastCat;
        for (const auto &issue : allIssues) {
            if (quietMode && !issue.detected) continue;
            if (!quietMode && issue.category != lastCat) {
                out << "\n  ── " << issue.category << " ──\n";
                lastCat = issue.category;
            }
            printIssueText(issue, quietMode);
        }

        // Summary
        int issuesFound = 0;
        for (const auto &i : allIssues) if (i.detected) issuesFound++;

        out << "\n";
        if (issuesFound == 0) {
            out << "  \033[32mAll " << allIssues.size() << " checks passed.\033[0m\n\n";
        } else {
            out << "  \033[33m" << issuesFound << " issue(s) found out of " << allIssues.size() << " checks.\033[0m\n";
            if (!doFix) {
                out << "  Run with --fix to auto-fix issues.\n\n";
            }
        }
    }

    // Auto-fix if requested
    if (doFix) {
        int fixed = 0;
        int failed = 0;
        for (const QString &cat : categories) {
            HealthChecker *checker = checkerMap[cat](&app);
            checker->runCheck();  // Re-run to populate issues
            for (const auto &issue : checker->issues) {
                if (issue.detected && issue.autoFixable && !issue.fixCommand.isEmpty()) {
                    if (!jsonMode) out << "  Fixing: " << issue.title << "... ";
                    if (checker->fixIssue(issue.id)) {
                        fixed++;
                        if (!jsonMode) out << "\033[32mOK\033[0m\n";
                    } else {
                        failed++;
                        if (!jsonMode) out << "\033[31mFAILED\033[0m\n";
                    }
                }
            }
            delete checker;
        }

        if (jsonMode) {
            out << ",\n  \"fixes\": { \"applied\": " << fixed << ", \"failed\": " << failed << " }\n";
        } else {
            out << "\n  Fix summary: " << fixed << " applied, " << failed << " failed\n\n";
        }

        return failed > 0 ? 1 : 0;
    }

    int issuesFound = 0;
    for (const auto &i : allIssues) if (i.detected) issuesFound++;
    return issuesFound > 0 ? 1 : 0;
}
