#include <clopts.h>
#include <cstdio>
#include <filesystem>
#include <irgen.h>
#include <parser.h>
#include <sema.h>
#include <string>

using namespace command_line_opts;
std::string inputFile;
std::string outputFile;
bool        dumpAst;

void handleWarnings(std::string warning) {
    std::printf("TODO warning: %s\n", warning.c_str());
}
void setOutput(std::string path) {
    if (!outputFile.empty()) {
        std::fprintf(stderr, "Cannot have multiple output files\n");
        std::exit(1);
    }
    outputFile = path;
}
void handleDump(std::string tree) {
    if (tree == "ast") {
        dumpAst = true;
    } else {
        std::fprintf(stderr, "Invalid tree to dump `%s`\n", tree.c_str());
        std::exit(1);
    }
}
int unknownArg(std::string path) {
    if (std::filesystem::exists(path)) {
        if (!inputFile.empty()) {
            std::fprintf(stderr, "Cannot have multiple input files yet\n");
            return 1;
        }
        inputFile = path;
        return 0;
    }
    return 1;
}
clopts_opt_t clopts = {
    {{"-W", handleWarnings, false}, {"-o", setOutput, true}, {"-dump-", handleDump, false}},
    unknownArg};

int main(int argc, char** argv) {
    std::string contents;
    clopts.parse(argc, argv);
    contents                 = clopts.handleFile(inputFile);
    language::Lexer*  lexer  = new language::Lexer(contents);
    language::Parser* parser = new language::Parser(lexer);
    language::Sema*   sema   = new language::Sema(parser->getAst());
    language::Ast*    ast    = sema->getNewAst();
    if (dumpAst) {
        ast->print();
    }
    // language::IrGen*    irgen   = new language::IrGen(ast);
    // language::IrModule* _module = irgen->getModule();
    // _module->print();
}