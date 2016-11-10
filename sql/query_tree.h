#include "sql_priv.h"
#include "sql_class.h"
#include "table.h"
#include "records.h"

class Query_tree
{
    public:
        enum query_node_type
        {
            qntUndefined = 0,
            qntRestrict = 1,
            qntProject = 2,
            qntJoin = 3,
            qntSort = 4,
            qntDistinct = 5
        };

        enum join_con_type
        {
            jcUN = 0,
            jcNA = 1,
            jcON = 2,
            jcUS = 3
        };

        enum type_join
        {
            jnUNKNOWN = 0,
            jnINNER = 1,
            jnLEFTOUTER = 2,
            jnRIGHTOUTER = 3,
            jnFULLOUTER = 4,
            jnCROSSPRODUCT = 5,
            jnUNION = 6,
            jnINTERSET = 7
        };

        enum AggregateType
        {
            atNONE = 0,
            atCOUNT = 1
        };

        struct query_node
        {
            query_node();
            ~query_node();
            int nodeid;
            int parent_nodeid;
            bool sub_query;
            bool child;
            query_node_type node_type;
            type_join join_type;
            join_con_type join_cond;
            Item *where_expr;
            Item *join_expr;
            TABLE_LIST *relations[4];
            bool preempt_pipeline;
            List<Item> *fields;
            query_node *left;
            query_node *right;
        };

        query_node *root;

        ~Query_tree(void);
        void ShowPlan(query_node *QN, bool PrintOnRight);
};
