#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <odbcinst.h>
#include <odbcinstext.h>

SQLHENV V_OD_Env;   // ODBC环境句柄
long V_OD_erg;      // 函数结果
SQLHDBC V_OD_hdbc;  // 句柄连接
SQLHDBC V_OD_hstmt; // 句柄语句
char V_OD_stat[10]; // SQL状态
SQLINTEGER V_OD_err, V_OD_rowanz;
SQLINTEGER V_OD_lsh;
TIMESTAMP_STRUCT V_OD_date;
SQLSMALLINT V_OD_mlen, V_OD_colanz;
char V_OD_msg[200], V_OD_bh[200], V_OD_mc[200], V_OD_cjyh[200], V_OD_syzt[200];

BOOL ODBCInit()
{
    // 分配环境句柄和注册版本
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &V_OD_Env);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("错误分配句柄 \n");
        return 0;
    }
    V_OD_erg = SQLSetEnvAttr(V_OD_Env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("错误设置环境 \n");
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    // 分配连接句柄，设置超时
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_DBC, V_OD_Env, &V_OD_hdbc);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("错误分配HDB  %ld\n", V_OD_erg);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    SQLSetConnectAttr(V_OD_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);
    return 1;
}

BOOL ODBCClose()
{
    SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt);
    SQLDisconnect(V_OD_hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
    printf("数据库关闭!\n");
    return 1;
}

BOOL ODBCMySQLConnect(char *DataSource, char *user, char *password)
{
    // 连接到数据源
    V_OD_erg = SQLConnect(V_OD_hdbc, (SQLCHAR *)DataSource, SQL_NTS,
                          (SQLCHAR *)user, SQL_NTS,
                          (SQLCHAR *)password, SQL_NTS);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("错误 SQLConnect %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1,
                      (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    printf("数据源%s已连接!\n", DataSource);
    return 1;
}

BOOL ODBCTables(char *dbname)
{
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc, &V_OD_hstmt);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("句柄分配错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    char sqlquery[128];
    sprintf(sqlquery, "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE table_schema=\'%s\'", dbname);
    SQLBindCol(V_OD_hstmt, 3, SQL_C_CHAR, &V_OD_bh, 150, (long *)&V_OD_err);
    V_OD_erg = SQLExecDirect(V_OD_hstmt, (SQLCHAR *)sqlquery, SQL_NTS);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("选择错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    V_OD_erg = SQLFetch(V_OD_hstmt);
    printf("%s数据库中有以下数据表：\n", dbname);
    int count = 0;
    if (V_OD_erg != SQL_NO_DATA)
    {
        printf("╒════════════════════════╕\n");
        printf("│ %22s │\n", V_OD_bh);
        V_OD_erg = SQLFetch(V_OD_hstmt);
        count++;

        while (V_OD_erg != SQL_NO_DATA)
        {
            printf("├────────────────────────┤\n");
            printf("│ %22s │\n", V_OD_bh);
            V_OD_erg = SQLFetch(V_OD_hstmt);
            count++;
        };
        printf("└────────────────────────┘\n");
    }
    printf("共有 %d 张数据表。\n", count);
    return 1;
}

BOOL ODBCDBs()
{
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc, &V_OD_hstmt);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("句柄分配错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    char sqlquery[128];
    sprintf(sqlquery, "select database()");
    SQLBindCol(V_OD_hstmt, 1, SQL_C_CHAR, &V_OD_bh, 150, (long *)&V_OD_err);
    V_OD_erg = SQLExecDirect(V_OD_hstmt, (SQLCHAR *)sqlquery, SQL_NTS);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("选择错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    V_OD_erg = SQLFetch(V_OD_hstmt);
    printf("MySQL中有以下数据库：\n");
    int count = 0;
    if (V_OD_erg != SQL_NO_DATA)
    {
        printf("╒════════════════════╕\n");
        printf("│ %18s │\n", V_OD_bh);
        V_OD_erg = SQLFetch(V_OD_hstmt);
        count++;
    }
    while (V_OD_erg != SQL_NO_DATA)
    {
        printf("├────────────────────┤\n");
        printf("│ %18s │\n", V_OD_bh);
        V_OD_erg = SQLFetch(V_OD_hstmt);
        count++;
    };
    printf("└────────────────────┘\n");
    printf("共有 %d 个数据库。\n", count);
    return 1;
}

BOOL ODBCTableColumn(char *Schema, char *table)
{
    //句柄
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc, &V_OD_hstmt);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("句柄分配错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    //读列结构
    SQLCHAR CatalogName[256] = "";
    SQLCHAR SchemaName[256];
    strcpy((char *)SchemaName, Schema);
    SQLCHAR TableName[256];
    strcpy((char *)TableName, table);
    SQLCHAR cn[256];
    SQLSMALLINT length1 = 0;
    SQLSMALLINT length2 = 0;//strlen((char *)SchemaName);
    SQLSMALLINT length3 = strlen(table);
    SQLSMALLINT length4 = 0;
    SQLHSTMT hstmt;
    char n1[150] = "default";
    char n2[150] = "default";
    SQLINTEGER n3 = 0;
    SQLRETURN aa;
    //SQLSetStmtAttr(V_OD_hstmt, SQL_ATTR_METADATA_ID, (SQLPOINTER)SQL_TRUE, 0);
    aa = SQLColumns(V_OD_hstmt, NULL, length1, NULL, length2, TableName, length3, NULL, length4);
    SQLCHAR state[6] = "origin";
    SQLCHAR errmsg[100] = "default";
    SQLSMALLINT errlen = 0;
    SQLINTEGER nerr = 0;
    SQLINTEGER c = 0;
    if (aa != SQL_SUCCESS)
    {
        SQLGetDiagRec(SQL_HANDLE_STMT, V_OD_hstmt, 1, state, &nerr, errmsg, 100, &errlen);
        printf("ERROR : %s : %s\n", state, errmsg);
    }
    V_OD_erg = SQLRowCount(V_OD_hstmt, (long *)&V_OD_rowanz);
    SQLBindCol(V_OD_hstmt, 4, SQL_C_TCHAR, &n1, 150, (long *)&V_OD_err);
    SQLBindCol(V_OD_hstmt, 6, SQL_C_CHAR, &n2, 150, (long *)&V_OD_err);
    SQLBindCol(V_OD_hstmt, 7, SQL_C_LONG, &n3, 150, (long *)&V_OD_err);
    V_OD_erg = SQLFetch(V_OD_hstmt);
    int Aid = V_OD_rowanz;
    char Acol[100][128];
    char Atype[100][128];
    int Alen[100];
    int Achar[100];
    int y = 1;
    int l = 0;
    int l2 = 0;
    while (y <= V_OD_rowanz)
    {
        strcpy(Acol[y - 1], n1);
        strcpy(Atype[y - 1], n2);
        Alen[y - 1] = strlen(n1);
        Achar[y - 1] = 0;
        int j;

        int k = 0;
        for (j = 0; j < strlen(n1); j++)
        {
            if (*(n1 + j) & 0x80)
            {
                k++;
            }
        }
        Achar[y - 1] = k / 3;
        if (l < Alen[y - 1] - Achar[y - 1])
        {
            l = Alen[y - 1] - Achar[y - 1];
        }
        if (l2 < strlen(n2))
        {
            l2 = strlen(n2);
        }
        V_OD_erg = SQLFetch(V_OD_hstmt);
        y++;
    }
    aa = SQLColumns(V_OD_hstmt, NULL, length1, SchemaName, length2, TableName, length3, NULL, length4);
    V_OD_erg = SQLFetch(V_OD_hstmt);
    y = 1;
    printf("\n数据表列结构：\n\n");
    printf("╒══╤");
    int i = 0;
    for (i = 0; i < l; i++)
    {
        printf("═");
    }
    char lm[8] = "列名";
    char lx[8] = "类型";
    printf("╤");
    for (i = 0; i < l2; i++)
    {
        printf("═");
    }
    printf("╕\n│id│%-*s│%-*s│\n╞══╪", l + 2, lm, l2 + 2, lx);
    i = 0;
    for (i = 0; i < l; i++)
    {
        printf("═");
    }
    printf("╪");
    for (i = 0; i < l2; i++)
    {
        printf("═");
    }
    printf("╡\n");
    while (y <= V_OD_rowanz)
    {
        if (y != 1)
        {
            printf("├──┼");
            i = 0;
            for (i = 0; i < l; i++)
            {
                printf("─");
            }
            printf("┼");
            for (i = 0; i < l2; i++)
            {
                printf("─");
            }
            printf("┤\n");
        }
        printf("│%-2d│%-*s│%-*s│\n", y, l, n1, l2, n2);
        V_OD_erg = SQLFetch(V_OD_hstmt);
        y++;
    };
    printf("└──┴");
    i = 0;
    for (i = 0; i < l; i++)
    {
        printf("─");
    }
    printf("┴");
    for (i = 0; i < l2; i++)
    {
        printf("─");
    }
    printf("┘\n");
    return 1;
}

BOOL ODBCSelect(char *Schema, char *table)
{
    //句柄
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc, &V_OD_hstmt);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("句柄分配错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    //读列结构
    SQLCHAR CatalogName[256] = "";
    SQLCHAR SchemaName[256];
    strcpy((char *)SchemaName, Schema);
    SQLCHAR TableName[256];
    strcpy((char *)TableName, table);
    SQLCHAR cn[256];
    SQLSMALLINT length1 = 0;
    SQLSMALLINT length2 = 0;//strlen((char *)SchemaName);
    SQLSMALLINT length3 = strlen(table);
    SQLSMALLINT length4 = 0;
    SQLHSTMT hstmt;
    char n1[150] = "default";
    char n2[150] = "default";
    SQLINTEGER n3 = 0;
    SQLRETURN aa;
    SQLSetStmtAttr(V_OD_hstmt, SQL_ATTR_METADATA_ID, (SQLPOINTER)SQL_TRUE, 0);
    aa = SQLColumns(V_OD_hstmt, NULL, length1, NULL, length2, TableName, length3, NULL, length4);
    SQLCHAR state[6] = "origin";
    SQLCHAR errmsg[100] = "default";
    SQLSMALLINT errlen = 0;
    SQLINTEGER nerr = 0;
    SQLINTEGER c = 0;
    if (aa != SQL_SUCCESS)
    {
        SQLGetDiagRec(SQL_HANDLE_STMT, V_OD_hstmt, 1, state, &nerr, errmsg, 100, &errlen);
        printf("ERROR : %s : %s\n", state, errmsg);
    }
    V_OD_erg = SQLRowCount(V_OD_hstmt, (long *)&V_OD_rowanz);
    SQLBindCol(V_OD_hstmt, 4, SQL_C_TCHAR, &n1, 150, (long *)&V_OD_err);
    SQLBindCol(V_OD_hstmt, 6, SQL_C_CHAR, &n2, 150, (long *)&V_OD_err);
    SQLBindCol(V_OD_hstmt, 7, SQL_C_LONG, &n3, 150, (long *)&V_OD_err);
    V_OD_erg = SQLFetch(V_OD_hstmt);
    int y = 1;
    int Aid = V_OD_rowanz;
    char Acol[100][128];
    char Atype[100][128];
    int Alen[100];
    int Achar[100];
    while (y <= V_OD_rowanz)
    {
        strcpy(Acol[y - 1], n1);
        strcpy(Atype[y - 1], n2);
        Alen[y - 1] = strlen(n1);
        Achar[y - 1] = 0;
        int k = 0;
        int j = 0;
        for (j = 0; j < strlen(n1); j++)
        {
            if (*(n1 + j) & 0x80)
            {
                k++;
            }
        }
        Achar[y - 1] = k / 3;
        Alen[y - 1] = Alen[y - 1] - Achar[y - 1];
        V_OD_erg = SQLFetch(V_OD_hstmt);
        y++;
    };

    // 开始读数据
    char sqlquery[128];
    sprintf(sqlquery, "SELECT * FROM `%s`", table);
    V_OD_erg = SQLExecDirect(V_OD_hstmt, (SQLCHAR *)sqlquery, SQL_NTS);
    if (V_OD_erg != SQL_SUCCESS)
    {
        SQLGetDiagRec(SQL_HANDLE_STMT, V_OD_hstmt, 1, state, &nerr, errmsg, 100, &errlen);
        printf("ERROR : %s : %s\n", state, errmsg);
    }
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("选择错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    SQLSMALLINT BufferLength = 128;
    char vc[100][128];
    int vi[100];
    float vf[100];
    int j = 0;
    for (; j < 100; j++)
    {
        strcpy(vc[j], "");
        vi[j] = 0;
        vf[j] = 0;
    }
    TIMESTAMP_STRUCT vd[100];
    int i = 0;
    //自动绑定
    for (; i < Aid; i++)
    {
        if (strcmp(Atype[i], "varchar") == 0)
        {
            SQLBindCol(V_OD_hstmt, i + 1, SQL_C_CHAR, &vc[i], BufferLength, (long *)&V_OD_err);
        }
        else if (strcmp(Atype[i], "char") == 0)
        {
            SQLBindCol(V_OD_hstmt, i + 1, SQL_C_CHAR, &vc[i], BufferLength, (long *)&V_OD_err);
        }
        else if (strcmp(Atype[i], "float") == 0)
        {
            SQLBindCol(V_OD_hstmt, i + 1, SQL_C_FLOAT, &vf[i], BufferLength, (long *)&V_OD_err);
        }
        else if (strcmp(Atype[i], "integer") == 0)
        {
            SQLBindCol(V_OD_hstmt, i + 1, SQL_C_LONG, &vi[i], BufferLength, (long *)&V_OD_err);
        }
        else if (strcmp(Atype[i], "datetime") == 0)
        {
            SQLBindCol(V_OD_hstmt, i + 1, SQL_C_TIMESTAMP, &vd[i], BufferLength, (long *)&V_OD_err);
        }
    }
    //更新列宽度
    V_OD_erg = SQLFetch(V_OD_hstmt);
    while (V_OD_erg != SQL_NO_DATA)
    {
        i = 0;
        for (; i < Aid; i++)
        {
            if (strcmp(Atype[i], "varchar") == 0)
            {
                if (strlen(vc[i]) > Alen[i])
                {
                    Alen[i] = strlen(vc[i]);

                    int k = 0;
                    for (j = 0; j < strlen(vc[i]); j++)
                    {
                        if (*(vc[i] + j) & 0x80)
                        {
                            k++;
                        }
                    }
                    Achar[i] = k / 3;
                    Alen[i] = Alen[i] - Achar[i];
                }
            }
            else if (strcmp(Atype[i], "char") == 0)
            {
                if (strlen(vc[i]) > Alen[i])
                {
                    Alen[i] = strlen(vc[i]);

                    int k = 0;
                    for (j = 0; j < strlen(vc[i]); j++)
                    {
                        if (*(vc[i] + j) & 0x80)
                        {
                            k++;
                        }
                    }
                    Achar[i] = k / 3;
                    Alen[i] = Alen[i] - Achar[i];
                }
            }
            else if (strcmp(Atype[i], "float") == 0)
            {
                float t = vf[i];
                int f = 6;
                while (t > 1)
                {
                    t = t / 10;
                    f++;
                }
                if (f > Alen[i])
                {
                    Alen[i] = f;
                }
            }
            else if (strcmp(Atype[i], "integer") == 0)
            {
                int t = vi[i];
                int f = 0;
                while (t != 0)
                {
                    t = t / 10;
                    f++;
                }
                if (f > Alen[i])
                {
                    Alen[i] = f;
                }
            }
            else if (strcmp(Atype[i], "datetime") == 0)
            {
                Alen[i] = 19;
            }
        }
        V_OD_erg = SQLFetch(V_OD_hstmt);
    };
    V_OD_erg = SQLExecDirect(V_OD_hstmt, (SQLCHAR *)sqlquery, SQL_NTS);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("选择错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return 0;
    }
    i = 0;
    int k = 0;
    //显示数据
    printf("\n数据表内容：\n\n╒");
    for (i = 0; i < Aid; i++)
    {
        for (k = 0; k < Alen[i]; k++)
        {
            printf("═");
        }
        if (i < Aid - 1)
            printf("╤");
        else
            printf("╕\n");
    }
    for (i = 0; i < Aid; i++)
    {
        if (strcmp(Atype[i], "datetime") == 0)
        {
            printf("│%-19s", Acol[i]);
        }
        else
        {
            printf("│%-*s", Alen[i], Acol[i]);
        }
    }
    printf("│\n╞");
    for (i = 0; i < Aid; i++)
    {
        for (k = 0; k < Alen[i]; k++)
        {
            printf("═");
        }
        if (i < Aid - 1)
            printf("╪");
        else
            printf("╡\n");
    }
    V_OD_erg = SQLFetch(V_OD_hstmt);
    int p = 0;
    while (V_OD_erg != SQL_NO_DATA)
    {
        if (p != 0)
        {
            printf("├");
            for (i = 0; i < Aid; i++)
            {
                for (k = 0; k < Alen[i]; k++)
                {
                    printf("─");
                }
                if (i < Aid - 1)
                    printf("┼");
                else
                    printf("┤\n");
            }
        }
        p++;
        i = 0;
        for (; i < Aid; i++)
        {
            if (strcmp(Atype[i], "varchar") == 0)
            {
                int j;
                int k = 0;
                for (j = 0; j < strlen(vc[i]); j++)
                {
                    if (*(vc[i] + j) & 0x80)
                    {
                        k++;
                    }
                }
                printf("│%-*s", Alen[i] + k / 3, vc[i]); //k/3是为了匹配中英文混杂时造成的无法对齐
            }
            else if (strcmp(Atype[i], "char") == 0)
            {
                int j;
                int k = 0;
                for (j = 0; j < strlen(vc[i]); j++)
                {
                    if (*(vc[i] + j) & 0x80)
                    {
                        k++;
                    }
                }
                printf("│%-*s", Alen[i] + k / 3, vc[i]);
            }
            else if (strcmp(Atype[i], "float") == 0)
            {
                printf("│%*.4f", Alen[i], vf[i]);
            }
            else if (strcmp(Atype[i], "integer") == 0)
            {
                printf("│%-*d", Alen[i], vi[i]);
            }
            else if (strcmp(Atype[i], "datetime") == 0)
            {
                printf("│%04d-%02d-%02d %02d:%02d:%02d", vd[i].year, vd[i].month, vd[i].day, vd[i].hour, vd[i].minute, vd[i].second);
            }
        }
        printf("│\n");
        V_OD_erg = SQLFetch(V_OD_hstmt);
    };
    printf("└");
    for (i = 0; i < Aid; i++)
    {
        for (k = 0; k < Alen[i]; k++)
        {
            printf("─");
        }
        if (i < Aid - 1)
            printf("┴");
        else
            printf("┘\n");
    }
    return 1;
}

void ODBCInsertOrUpdate(char *sqlquery)
{
    // 分配sql连接句柄，并执行查询
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc, &V_OD_hstmt);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("句柄分配错误 %ld", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return;
    }
    // 执行sql
    V_OD_erg = SQLExecDirect(V_OD_hstmt, (SQLCHAR *)sqlquery, SQL_NTS);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
    {
        printf("选择错误 %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR *)V_OD_stat, &V_OD_err, (SQLCHAR *)V_OD_msg, 100, &V_OD_mlen);
        printf("%s (%d)\n", V_OD_msg, V_OD_err);
        SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
        return;
    }
    printf("语句执行成功!\n");
    return;
}

int main(int argc, char *argv[])
{
    if (strcmp(argv[1], "-h") == 0)
    {
        printf("ODBC_SQL帮助：\n\n");
        printf("-d       查询数据库。\n");
        printf("-t       查询数据表，需要输入数据库名。\n");
        printf("-s       查询数据，需要输入表名。\n");
        printf("-c       查询数据表列结构，需要输入表名。\n");
        printf("-i或-u   插入或更新，需要输入sql语句。\n");
        printf("-v       查询版本。\n\n");
        printf("             CodeBy@LLZ\n");
    }
    else if (strcmp(argv[1], "-v") == 0)
    {
        printf("\nODBC_SQL 1.7.10.20     CodeBy@LLZ\n\n");
    }
    else
    {
        printf("%d\n", argc);
        char DataSource[16] = "testdb";
        char Database[16] = "fsu";
        char User[16] = "root";
        char PW[16] = "1234567";
        char query[64];
        char query2[64];
        //默认数据源
        sprintf(query, "%s", argv[2]);
        sprintf(query2, "%s", argv[2]);
        ODBCInit();
        ODBCMySQLConnect(DataSource, User, PW);
        if (strcmp(argv[1], "-s") == 0)
        {
            ODBCSelect(Database, query);
        }
        else if (strcmp(argv[1], "-i") == 0)
        {
            ODBCInsertOrUpdate(query2);
        }
        else if (strcmp(argv[1], "-u") == 0)
        {
            ODBCInsertOrUpdate(query2);
        }
        else if (strcmp(argv[1], "-t") == 0)
        {
            ODBCTables(query2);
        }
        else if (strcmp(argv[1], "-d") == 0)
        {
            ODBCDBs();
        }
        else if (strcmp(argv[1], "-c") == 0)
        {
            ODBCTableColumn(Database, query);
        }
        ODBCClose();
    }
    return 0;
}
