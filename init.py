import os
import argparse

parser = argparse.ArgumentParser("init ev_sdk")
parser.add_argument("-n", "--names", nargs="+", required=False, default=[], help="name of classes",)
args =  parser.parse_args()

if __name__ == "__main__":
    with open("/usr/local/ev_sdk/config/coco.names", "w") as f:
        f.writelines([n+"\n" for n in args.names])
    
    with open("/usr/local/ev_sdk/config/algo_config.json", "r+", encoding="utf-8") as f:
        totallines = f.readlines()
        lines = totallines[:20]
        for i, n in enumerate(args.names):
            lines.append("    \"mark_text_en_{}\": \"{}\" , \n".format(i, n))
            lines.append("    \"mark_text_zh_{}\": \"{}\" , \n".format(i, n))
        lines += totallines[20:]
    with open("/usr/local/ev_sdk/config/algo_config.json", "w",  encoding="utf-8") as f:
        f.writelines(lines)

    with open("/usr/local/ev_sdk/include/Configuration.hpp", "r+", encoding="utf-8") as f:
        totallines = f.readlines()
        lines = totallines[:47]
        for i, n in enumerate(args.names):
            lines.append("    std::map<std::string, std::string> targetRectTextMap_{} = {{ {{\"en\", \"{}\"}}, {{\"zh\", \"{}\"}} }}; \n".format(i, n, n))
        lines += totallines[47:]
    with open("/usr/local/ev_sdk/include/Configuration.hpp", "w",  encoding="utf-8") as f:
        f.writelines(lines)

