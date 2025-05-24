#if !defined(_CLOPTS_H_)
#define _CLOPTS_H_
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace command_line_opts
{
struct clopts_arg_t
{
    std::string                      prefix;
    std::function<void(std::string)> function;
    bool                             takesSpace;
};
class clopts_opt_t
{
  public:
    clopts_opt_t(std::vector<clopts_arg_t> args)
    {
        this->mArgs   = args;
        this->mUnkown = {};
    }
    clopts_opt_t(std::vector<clopts_arg_t> args, std::function<int(std::string)> unknownArg)
    {
        this->mArgs   = args;
        this->mUnkown = unknownArg;
    }
    void parse(int argc, char** argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string strArg = std::string(argv[i]);
            bool        found  = false;
            for (clopts_arg_t arg : mArgs)
            {
                if (found)
                {
                    break;
                }
                if (strArg.starts_with(arg.prefix))
                {
                    char* opt = (char*)((size_t)strArg.c_str() + arg.prefix.size());
                    if (strArg == arg.prefix && arg.takesSpace)
                    {
                        i++;
                        opt = argv[i];
                    }
                    if (strlen(opt) != 0) {
                        arg.function(std::string(opt));
                        found = true;
                    }
                }
            }
            if (!found && mUnkown.has_value() && mUnkown.value()(strArg))
            {
                fprintf(stderr, "Unrecognized option: `%s`\n", strArg.c_str());
                exit(1);
            }
        }
    }
    std::string handleFile(std::string path)
    {
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (!f)
        {
            fprintf(stderr, "%s `%s`\n", std::strerror(errno), path.c_str());
            exit(1);
        }
        std::fseek(f, 0, SEEK_END);
        size_t length = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        char* buffer = new char[length]();
        std::fread(buffer, 1, length, f);
        std::fclose(f);
        std::string ret = std::string(buffer, length);
        delete[] buffer;
        return ret;
    }

  private:
    std::vector<clopts_arg_t>                      mArgs;
    std::optional<std::function<int(std::string)>> mUnkown;
};
}; // namespace command_line_opts

#endif // _CLOPTS_H_
