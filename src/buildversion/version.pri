DISTFILES += \
    $$PWD/version.h.txt \
    $$PWD/version.txt


#引用：https://zhuanlan.zhihu.com/p/550295522
#定义版本自动升级函数
#version.txt文件：
#读取旧版本号，保存新版本号
#版本号格式类似：1.0.0.1
#返回值为升级后的版本号
defineReplace(butianyun_update_version) {
    OLD_VERSION = $$cat($$PWD/buildversion/version.txt, lines)
    VERSION_NUMBERS = $$split(OLD_VERSION, .)
#    VA = $$take_first(VERSION_NUMBERS)
    VA = 0
    VB = $$take_first(VERSION_NUMBERS)
    VC = $$take_first(VERSION_NUMBERS)
    VD = $$take_first(VERSION_NUMBERS)
    VA=1
    #如果没有读取到合适的版本号则自动初始化
    #初始版本号为1.0.0.1
    lessThan(VA, 1) {
      VA = 1
      VOS += VA
    }
    lessThan(VB, 0) {
      VB = 0
      VOS += VB
    }
    lessThan(VC, 0) {
      VC = 0
      VOS += VC
    }
    lessThan(VD, 1) {
      VD = 1
      VOS += VD
    }
    #如果配置的版本号有效则升级版本号
    isEmpty(VOS) {
        VM = 1000
        lessThan(VD, $$VM) {
          VD = $$num_add($$VD, 1)
        }
        else {
          VD = 1
          lessThan(VC, $$VM) {
              VC = $$num_add($$VC, 1)
          }
          else {
              VC = 1
              lessThan(VB, $$VM) {
                  VB = $$num_add($$VB, 1)
              }
              else {
                  VB = 1
                  VA = $$num_add($$VA, 1)
              }
          }
        }
    }
    S = .
    NEW_VERSION =$$VB$$S$$VC$$S$$VD
    message($$OLD_VERSION => $$NEW_VERSION)
    #将升级后的版本号写入到文件中
    BUTIANYUN_VERSION_H_TXT = $$cat($$PWD/buildversion/version.h.txt, lines)
    BUTIANYUN_VERSION_H = $$replace(BUTIANYUN_VERSION_H_TXT, butianyun_version_number, $$NEW_VERSION)
    message($$BUTIANYUN_VERSION_H)
    message($$NEW_VERSION)
    FILE_PATH=$$PWD/buildversion/version.h
#    write_file(FILE_PATH, BUTIANYUN_VERSION_H)
    !system(echo $$BUTIANYUN_VERSION_H > $$FILE_PATH) {
      warning(Cant create a file)
    }
    FILE_PATH=$$PWD/buildversion/version.txt
#    write_file(FILE_PATH, NEW_VERSION)
    !system(echo $$NEW_VERSION > $$FILE_PATH) {
      warning(Cant create a file)
    }
    return ($$VERSION)
}

HEADERS += \
    $$PWD/version.h
    #调用版本号自动升级函数

