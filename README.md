## 说明

参照Expert MySQL代码,学习如何添加Spartan引擎以及如何添加新的SQL命令

在5.1.45版本也实现类似的功能,具体参照[5.1.45版本的Demo](https://github.com/dolphinsboy/mysql-5.1.45/blob/master/README.md)

## 不同之处

**在5.6.29版本中添加新的引擎**

相对比较简单,只需要修改CMakeList.txt文件即可

静态编译的模式

```
SET(SPARTAN_PLUGIN_STATIC "ha_spartan")
SET(SPARTAN_PLUGIN_MANDATORY TRUE)
SET(SPARTAN_SOURCES
    ha_spartan.cc ha_spartan.h
    spartan_data.cc spartan_data.h
    spartan_index.cc spartan_index.h
    )
MYSQL_ADD_PLUGIN(SPARTAN ${SPARTAN_SOURCES} STORAGE_ENGINE MANDATORY)
```
