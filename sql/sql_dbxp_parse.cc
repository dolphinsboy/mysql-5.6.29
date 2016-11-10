#include "query_tree.h"

Query_tree *build_query_tree(THD *thd, LEX *lex, TABLE_LIST *tables)
{
    DBUG_ENTER("build_query_tree");
    Query_tree *qt = new Query_tree();
    Query_tree::query_node *qn = new Query_tree::query_node();

    TABLE_LIST *table;
    int i = 0;
    int num_tables = 0;

    qn->parent_nodeid = -1;
    qn->child = false;
    qn->join_type = (Query_tree::type_join)0;
    qn->nodeid = 0;
    qn->node_type = (Query_tree::query_node_type)2;
    qn->left = 0;
    qn->right = 0;

    i = 0;
    for(table = tables; table; table = table->next_local)
    {
        num_tables++;
        qn->relations[i] = table;
        i++;
    }

    qn->fields = &lex->select_lex.item_list;
    if(num_tables>0)
    {
        for(table = tables; table; table = table->next_local)
        {
            if(((Item*)table->join_cond() != 0) && (qn->join_expr == 0))
                qn->join_expr = (Item*)table->join_cond();
        }
    }

    qn->where_expr = lex->select_lex.where;
    qt->root = qn;
    DBUG_RETURN(qt);
}

int DBXP_select_command(THD *thd)
{
    DBUG_ENTER("DBXP_select_command");
    Query_tree *qt = build_query_tree(thd, thd->lex,
            (TABLE_LIST*)thd->lex->select_lex.table_list.first);

    List<Item> field_list;
    Protocol *protocol = thd->protocol;
    field_list.push_back(
            new Item_empty_string(
                "Database Experiment Project (DBXP)", 
                40));
    field_list.push_back(new Item_return_int("Id",2, MYSQL_TYPE_LONGLONG));
    if(protocol->send_result_set_metadata(&field_list,
                Protocol::SEND_NUM_ROWS|Protocol::SEND_EOF))
        DBUG_RETURN(TRUE);
    protocol->prepare_for_resend();
    protocol->store("Query tree was build.", system_charset_info);
    protocol->store((longlong)qt->root->nodeid);

    if(protocol->write())
        DBUG_RETURN(TRUE);
    my_eof(thd);
    DBUG_RETURN(0);
}

int DBXP_explain_select_command(THD *thd)
{

    DBUG_ENTER("DBXP_select_command");
    Query_tree *qt = build_query_tree(thd, thd->lex,
            (TABLE_LIST*)thd->lex->select_lex.table_list.first);

    List<Item> field_list;
    Protocol *protocol = thd->protocol;
    field_list.push_back(
            new Item_empty_string(
                "Database Experiment Project (DBXP)", 
                40));
    field_list.push_back(new Item_int("id",10));
    if(protocol->send_result_set_metadata(&field_list,
                Protocol::SEND_NUM_ROWS|Protocol::SEND_EOF))
        DBUG_RETURN(TRUE);
    protocol->prepare_for_resend();
    protocol->store("Query tree was build.", system_charset_info);
    protocol->store((longlong)qt->root->nodeid);

    if(protocol->write())
        DBUG_RETURN(TRUE);
    my_eof(thd);
    DBUG_RETURN(0);
}
