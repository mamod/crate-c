var sock = process.socket;
var crate_db = {};

/* global statements */
crate_db.statements_num = 1;
crate_db.statements = {};

crate_db.connect = function (resultObject){
    if (!crate_db.fd){
        var fd = sock.socket(sock.AF_INET, sock.SOCK_STREAM, 0);
        var peeraddr = sock.pton('127.0.0.1', 4200);
        if (peeraddr === null){
            resultObject.errorMsg = "couldn't construct an address from ip ...";
            resultObject.errorCode = process.errno;
            return null;
        }

        var conn = sock.connect(fd, peeraddr);
        if (conn === null){
            resultObject.errorMsg = "couldn't connect to ";
            resultObject.errorCode = process.errno;
            return null;
        }

        crate_db.fd = fd;
        crate_db.peeraddr = peeraddr;
    }

    return crate_db.fd;
};

crate_db.execute_sql = function(sql, args, resultObject){
    resultObject = resultObject || crate_db;
    resultObject.sql_query = sql;

    var query = { stmt : sql };

    if (args){ query.args = args; }

    var query_string = JSON.stringify(query);
    // print(query_string);
    var req =  "POST /_sql HTTP/1.1\r\n";
        req += "Content-Length: " +  query_string.length + "\r\n";
        req += "\r\n";
        req += query_string;
    
    
    var fd = crate_db.connect(resultObject);
    if (fd === null){
        return -1;
    }
    
    var readlength = 8 * 1024;
    sock.send(fd, req, req.length, 0);
    
    var response = '';
    while (1){
        var data = sock.recv(fd, readlength);
        response += data || '';
        if (!data || data.length < readlength){
            break
        }
    }
    
    var res = response.split("\r\n\r\n");
    // print(res[1]);

    var responseBody = JSON.parse(res[1]);

    if (responseBody.error){
        resultObject.errorMsg = responseBody.error.message;
        resultObject.errorCode = responseBody.error.code;
        return -1;
    }

    resultObject.headers = res[0];
    resultObject.raw     = res[1];
    resultObject.data    = JSON.parse(res[1]);
    resultObject.cols    = resultObject.data.cols;

    //parse field names into object
    resultObject.colNames = {};
    for (var i = 0; i < resultObject.cols.length; i++){
        resultObject.colNames[resultObject.cols[i]] = i;
    }

    return  resultObject.data.rowcount;
};

crate_db.error_message = function(stmt){
    /* we need global error not statement specific */
    if (stmt === null){
        return crate_db.errorMsg;
    }

    var statement = crate_db.statements[stmt];
    return statement.errorMsg;
};

crate_db.error_code = function(stmt){
    /* we need global error not statement specific */
    if (stmt === null){
        return crate_db.errorCode;
    }

    var statement = crate_db.statements[stmt];
    return statement.errorCode;
};

crate_db.close = function(){
    if (crate_db.fd){
        sock.close(crate_db.fd);
    }
};

crate_db.stmt_init  = function(){
    var num = crate_db.statements_num++;
    crate_db.statements[num] = {
        binds : []
    };
    return num;
};

crate_db.stmt_query  = function(id, val){
    var statement = crate_db.statements[id];
    statement.query = val;
    return 1;
};

crate_db.stmt_bind  = function(id, val){
    var statement = crate_db.statements[id];
    statement.binds.push(val);
    return 1;
};

crate_db.stmt_execute  = function(id){
    var statement = crate_db.statements[id];
    var ret = crate_db.execute_sql(statement.query, statement.binds, statement);
    //reset binds
    statement.binds = [];
    return ret;
};

crate_db.stmt_close  = function(id){
    delete crate_db.statements[id];
    return 1;
};

crate_db.stmt_get_row_data = function(id, i, field){
    var statement = crate_db.statements[id];
    var fieldIndex = statement.colNames[field];
    var data = statement.data;
    var t = data.rows[i][fieldIndex];
    return t;
};

crate_db.get_row_data = function(i, field){
    var fieldIndex = crate_db.colNames[field];
    var data = crate_db.data;
    var t = data.rows[i][fieldIndex];
    return t;
};
