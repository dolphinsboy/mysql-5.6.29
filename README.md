## 一. 说明

参照Expert MySQL代码,学习如何添加Spartan引擎以及如何添加新的SQL命令

在5.1.45版本也实现类似的功能,具体参照[5.1.45版本的Demo](https://github.com/dolphinsboy/mysql-5.1.45/blob/master/README.md)

## 二. 不同之处

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

## 三. 在information_schema库中添加相关表

### 1. 修改sql/handler.h添加表说明

```c
/*
  Make sure that the order of schema_tables and enum_schema_tables are the same.
*/
enum enum_schema_tables
{
  SCH_CHARSETS= 0,
  SCH_COLLATIONS,
  SCH_COLLATION_CHARACTER_SET_APPLICABILITY,
  SCH_COLUMNS,
  SCH_COLUMN_PRIVILEGES,
  /*BEGIN GUOSONG MODIFICATION*/
  SCH_DISKUSAGE,
  /*END GUOSONG MODIFICATION*/   
...
```

### 2.修改sql/sql_parse.cc中的prepare_schema_table

```c
int prepare_schema_table(THD *thd, LEX *lex, Table_ident *table_ident,
    ¦   ¦   ¦   ¦   ¦   ¦enum enum_schema_tables schema_table_idx)
{
  SELECT_LEX *schema_select_lex= NULL;
  DBUG_ENTER("prepare_schema_table");

  switch (schema_table_idx) {
/*BEGIN GUOSONG MODIFICATION*/
  case SCH_DISKUSAGE:
/*END GUOSONG MODIFICATION*/
  case SCH_SCHEMATA:
```

### 3.修改sql/sql_show.cc添加表对应的列说明

```c
/*BEGIN GUOSONG MODIFICATION*/
ST_FIELD_INFO disk_usage_fields_info[]=
{
    {"DATABASE", 40, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE},
    {"Size_in_bytes",21, MYSQL_TYPE_LONG, 0, 0, NULL, SKIP_OPEN_TABLE},
    {0, 0, MYSQL_TYPE_STRING, 0, 0, 0, SKIP_OPEN_TABLE}
};
/*END GUOSONG MODIFICATION*/
```

### 4.修改sql/sql_show.cc中的schema_tables数组

```c
ST_SCHEMA_TABLE schema_tables[]=
{
  {"CHARACTER_SETS", charsets_fields_info, create_schema_table, 
   fill_schema_charsets, make_character_sets_old_format, 0, -1, -1, 0, 0},
  {"COLLATIONS", collation_fields_info, create_schema_table, 
   fill_schema_collation, make_old_format, 0, -1, -1, 0, 0},
  {"COLLATION_CHARACTER_SET_APPLICABILITY", coll_charset_app_fields_info,
   create_schema_table, fill_schema_coll_charset_app, 0, 0, -1, -1, 0, 0},
  {"COLUMNS", columns_fields_info, create_schema_table, 
   get_all_tables, make_columns_old_format, get_schema_column_record, 1, 2, 0,
   OPTIMIZE_I_S_TABLE|OPEN_VIEW_FULL},
  {"COLUMN_PRIVILEGES", column_privileges_fields_info, create_schema_table,
   fill_schema_column_privileges, 0, 0, -1, -1, 0, 0},
  /*BEGIN GUOSONG MODIFICATION*/
  {"DISKUSAGE",disk_usage_fields_info, create_schema_table,
  fill_disk_usage, make_old_format, 0, -1, -1, 0, 0},
  /*END GUOSONG MODIFICATION*/
  {"ENGINES", engines_fields_info, create_schema_table,
   fill_schema_engines, make_old_format, 0, -1, -1, 0, 0}, 
```

**顺序和enum_shcema_tables对应**

### 5.在sql/sql_show.cc添加fill_disk_usage函数

```c
int fill_disk_usage(THD *thd, TABLE_LIST *tables, Item *cond)
{
    TABLE *table= tables->table;
    CHARSET_INFO *scs = system_charset_info;
    List<Item> field_list;
    List<LEX_STRING> dbs;
    LEX_STRING  *db_name;
    char *path;
    MY_DIR *dirp;
    FILEINFO *file;
    long long fsizes = 0;
    long long lsizes = 0;
    DBUG_ENTER("fill_disk_usage");
    find_files_result res = find_files(thd, &dbs, 0, mysql_data_home, 0, 1, NULL);
    if(res != FIND_FILES_OK)
        DBUG_RETURN(1);

    List_iterator_fast<LEX_STRING> it_dbs(dbs);
    path = (char*)my_malloc(PATH_MAX, MYF(MY_ZEROFILL));
    dirp = my_dir(mysql_data_home, MYF(MY_WANT_STAT));

    for(int i = 0; i < (int)dirp->number_off_files; i++)
    {
        file = dirp->dir_entry + i;
        if(strncasecmp(file->name, "ibdata",6) == 0)
            fsizes = fsizes + file->mystat->st_size;
        else if(strncasecmp(file->name, "ib", 2) == 0)
            lsizes = lsizes + file->mystat->st_size;
    }
    table->field[0]->store("InnoDB Tablespace",
        strlen("InnoDB Tablespace"),scs);
    table->field[1]->store((long long)fsizes, TRUE);
    if(schema_table_store_record(thd, table))
        DBUG_RETURN(1);
    table->field[0]->store("InnoDB Logs",
        strlen("InnoDB Logs"),scs);
    table->field[1]->store((long long)lsizes, TRUE);
    if(schema_table_store_record(thd, table))
        DBUG_RETURN(1);

    while((db_name = it_dbs++))
    {
        fsizes = 0;
        strcpy(path, mysql_data_home);
        strcat(path, "/");
        strcat(path, db_name->str);
        dirp = my_dir(path, MYF(MY_WANT_STAT));
        for(int i = 0; i < (int)dirp->number_off_files; i++)
        {
            file = dirp->dir_entry + i;
            fsizes = fsizes + file->mystat->st_size;
        }
        restore_record(table, s->default_values);
        table->field[0]->store(db_name->str,db_name->length, scs);
        table->field[1]->store((long long)fsizes, TRUE);

        if(schema_table_store_record(thd, table))
            DBUG_RETURN(1);
    }

    my_free(path);
    DBUG_RETURN(0);
}
```

### 6.结果展示

```sql

mysql> show disk_usage;
+--------------------+---------------+
| Database           | Size_in_bytes |
+--------------------+---------------+
| InnoDB Tablespace  |     104857600 |
| Innodb Logs        |     201326592 |
| mysql              |       1568529 |
| performance_schema |        493639 |
| test               |         35204 |
+--------------------+---------------+
5 rows in set (0.01 sec)

mysql> select * from information_schema.diskusage;
+--------------------+---------------+
| DATABASE           | Size_in_bytes |
+--------------------+---------------+
| InnoDB Tablespace  |     104857600 |
| InnoDB Logs        |     201326592 |
| mysql              |       1568529 |
| performance_schema |        493639 |
| test               |         35204 |
+--------------------+---------------+
5 rows in set (0.01 sec)

mysql> show create table information_schema.diskusage\G
*************************** 1. row ***************************
       Table: DISKUSAGE
Create Table: CREATE TEMPORARY TABLE `DISKUSAGE` (
  `DATABASE` varchar(40) NOT NULL DEFAULT '',
  `Size_in_bytes` int(21) NOT NULL DEFAULT '0'
) ENGINE=MEMORY DEFAULT CHARSET=utf8
```

## 四. 添加DBXP_SELECT查询

### 1.修改sql/lex.h添加DBXP_SELECT关键字

```c
static SYMBOL symbols[] = {   
    ...
    { "DBXP_SELECT", SYM(DBXP_SELECT_SYM)},
    ...
```

### 2.修改sql/sql_yacc.yy添加相关token

```c
%token DBXP_SELECT_SYM
```

**以下两个部分主要实现DBXP_SELECT作为单独命令必要条件**

在%type <NONE>中添加语句标示

```c
%type <NONE>
	...
	dbxp_select
	...
```

在statement中添加dbxp_select

```c
/* Verb clauses, except begin */
statement:
        | use
		| xa
	    /*BEGIN GUOSONG MODIFICATION*/
        | dbxp_select
        /*END GUOSONG MODIFICATION*/
```

添加语法解析对于的操作

```c
/*BEGIN GUOSONG DBXP MODIFICATION*/
dbxp_select:
    ¦   DBXP_SELECT_SYM
    ¦   {
    ¦   ¦   LEX *lex = Lex;
    ¦   ¦   lex->sql_command = SQLCOM_DBXP_SELECT;
    ¦   };

/*END GUOSONG DBXP MODIFICATION*/
```

### 3.添加SQLCOM_DBXP_SELECT对应的处理函数

#### 3.1 在sql/sql_cmd.h添加SQLCOM_DBXP_SELECT命令类型

```c
enum enum_sql_command {
	  /*BEGIN GUOSONG MODIFICATION*/
  	  SQLCOM_DBXP_SELECT,
      SQLCOM_DBXP_EXPLAIN_SELECT,
      /*END GUOSONG MODIFICATION*/
```

#### 3.2 修改sql/sql_parse.cc添加SQLCOM_DBXP_SELECT对应的处理分支

mysql_execute_command函数中添加SQLCOM_DBXP_SELECT对应的处理分支

```c
/*BEGIN GUOSONG DBXP MODIFICATION*/
  case SQLCOM_DBXP_SELECT:
  {
    List<Item> field_list;
    Protocol *protocol = thd->protocol;
    
    /*build fields to client*/
    field_list.push_back(new Item_int("Id",21));
    field_list.push_back(new Item_empty_string("LastName", 40));
    field_list.push_back(new Item_empty_string("FirstName",20));
    field_list.push_back(new Item_empty_string("Gender",2));
    if(protocol->send_result_set_metadata(&field_list,
    ¦   ¦   ¦   Protocol::SEND_NUM_ROWS| Protocol::SEND_EOF))
    ¦   DBUG_RETURN(TRUE);
    
    protocol->prepare_for_resend();

    protocol->store((long long)3);
    protocol->store("Guo", system_charset_info);
    protocol->store("Song", system_charset_info);
    protocol->store("M", system_charset_info);
    if(protocol->write())
    ¦   DBUG_RETURN(TRUE);
    
    my_eof(thd);
    break;
  }
/*END GUOSONG DBXP MODIFICATION*/

```
### 4.重新执行make以及make install

在5.6版本中,可以不需要执行下面的命令,make操作会自动执行

```
bison -y -d sql_yacc.yy
mv y.tab.h sql_yacc.h
mv y.tab.c sql_yacc.cc
./gen_lex_hash > ./lex_hash.h
```

### 5. Demo

```sql
mysql> DBXP_SELECT;
+---+----------+-----------+--------+
|   | LastName | FirstName | Gender |
+---+----------+-----------+--------+
| 3 | Guo      | Song      | M      |
+---+----------+-----------+--------+
1 row in set (0.00 sec)

mysql> DBXP_SELECT * from t;
ERROR 1064 (42000): You have an error in your SQL syntax
```
可以看到目前只支持DBXP_SELECT单独查询,是因为在语法部分只写了DBXP_SELECT,下面添加其他处理分支.


### 6. 完善

**增加其他SQL语句判断**

```c
/*BEGIN GUOSONG DBXP MODIFICATION*/
dbxp_select:
        DBXP_SELECT_SYM DBXP_select_options DBXP_select_item_list
            DBXP_select_from
        {
            LEX *lex = Lex;
            lex->sql_command = SQLCOM_DBXP_SELECT;
        };
DBXP_select_options:
        | DISTINCT
        {
            Select->options |= SELECT_DISTINCT;
        };
DBXP_select_item_list:
        | DBXP_select_item_list ',' select_item
        | select_item
        | '*'
        {
            THD *thd = YYTHD;
            Item *item = new(thd->mem_root)
                    Item_field(&thd->lex->current_select->context,
                               NULL, NULL, "*");
            if(item == NULL)
                MYSQL_YYABORT;
            if(add_item_to_list(thd,item))
                MYSQL_YYABORT;
            (thd->lex->current_select->with_wild)++;
        };
DBXP_select_from:
        FROM join_table_list DBXP_where_clause{};
DBXP_where_clause:
        /*empty*/ {Select->where=0;}
        | WHERE expr
        {
            SELECT_LEX *select = Select;
            select->where  = $2;
            if($2)
                $2->top_level_item();
        };
/*END GUOSONG DBXP MODIFICATION*/
```

**Demo**

```sql
mysql> DBXP_SELECT * from t;
+---+----------+-----------+--------+
|   | LastName | FirstName | Gender |
+---+----------+-----------+--------+
| 3 | Guo      | Song      | M      |
+---+----------+-----------+--------+
1 row in set (0.00 sec)

mysql> DBXP_SELECT distinct * from t;
+---+----------+-----------+--------+
|   | LastName | FirstName | Gender |
+---+----------+-----------+--------+
| 3 | Guo      | Song      | M      |
+---+----------+-----------+--------+
1 row in set (0.00 sec)

mysql> DBXP_SELECT distinct * from t where a=1;
+---+----------+-----------+--------+
|   | LastName | FirstName | Gender |
+---+----------+-----------+--------+
| 3 | Guo      | Song      | M      |
+---+----------+-----------+--------+
1 row in set (0.00 sec)

mysql> DBXP_SELECT distinct * from t where a=1 and b=2;
+---+----------+-----------+--------+
|   | LastName | FirstName | Gender |
+---+----------+-----------+--------+
| 3 | Guo      | Song      | M      |
+---+----------+-----------+--------+
1 row in set (0.00 sec)
```

