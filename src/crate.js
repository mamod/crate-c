var sock = process.socket;
var crate_db = {};

var NO_PIPE = sock.MSG_NOSIGNAL ? sock.MSG_NOSIGNAL : 0;

/* global statements */
crate_db.statements_num = 1;
crate_db.statements = {};

var  EWOULDBLOCK = errno.EWOULDBLOCK;

var ERRORS = {
    4000   : "The statement contains an invalid syntax or unsupported SQL statement",
    4001   : "The statement contains an invalid analyzer definition.",
    4002   : "The name of the table is invalid.",
    4003   : "Field type validation failed",
    4004   : "Possible feature not supported (yet)",
    4005   : "Alter table using a table alias is not supported.",
    4006   : "The used column alias is ambiguous.",
    4041   : "Unknown table.",
    4042   : "Unknown analyzer.",
    4043   : "Unknown column.",
    4044   : "Unknown type.",
    4045   : "Unknown schema.",
    4046   : "Unknown Partition.",
    4091   : "A document with the same primary key exists already.",
    4092   : "A VersionConflict. Might be thrown if an attempt was made to update the same document concurrently.",
    4093   : "A table with the same name exists already.",
    4094   : "The used table alias contains tables with different schema.",
    5000   : "Unhandled server error.",
    5001   : "The execution of one or more tasks failed.",
    5002   : "one or more shards are not available.",
    5003   : "the query failed on one or more shards",
    5030   : "the query was killed by a kill statement"
};

crate_db.connect = function (resultObject){

    if (!crate.options){
        crate.options = {
            keepAlive : 6000,
            servers : [
                '127.0.0.1:4200'
            ]
        };
    }

    if (!crate_db.fd){

        var peeraddr = sock.pton('127.0.0.1', 4200);
        if (peeraddr === null){
            resultObject.errorMsg = "couldn't construct an address from ip ...";
            resultObject.errorCode = process.errno;
            return null;
        }

        var fd = sock.socket(sock.AF_INET, sock.SOCK_STREAM, 0);
        if (!sock.nonblock(fd, 1)){
            throw new Error('sock block');
        }

        var conn = sock.connect(fd, peeraddr);
        var ret  = sock.can_write(fd, 6000);
        if (ret === null || ret === 0){
            sock.close(fd);
            resultObject.errorMsg = "connection timed out";
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
    var req  =  "POST /_sql HTTP/1.1\r\n";
        req += "Content-Length: " +  query_string.length + "\r\n";
        req += "\r\n";
        req += query_string;
    
    
    var fd = crate_db.connect(resultObject);

    if (fd === null){
        return -1;
    }
    
    var readlength = 8 * 1024;

    /* WRITING */
    while (1){
        var can_write = sock.can_write(fd, 6000);
        if (can_write === null){
            if (process.errno === EWOULDBLOCK){
                continue;
            }
        } else if (can_write === 0){
            resultObject.errorMsg  = "write timed out.";
            resultObject.errorCode = process.errno;
            return -1;
        }

        var sent = sock.send(fd, req, req.length, NO_PIPE);
        if (sent === null){
            if (process.errno === EWOULDBLOCK){
                continue;
            }
            //error, do we have other servers pool
        } else if (sent < req.length){
            req = req.substr(sent);
            continue;
        }
        break;
    }

    /* READING */
    var response = '';
    while (1){
        var can_read = sock.can_read(fd, 6000);
        if (can_read === null){
            if (process.errno === EWOULDBLOCK){
                continue;
            }
        } else if (can_read === 0){
            //timed out
            resultObject.errorMsg  = "Socket read timed out";
            resultObject.errorCode = process.errno;
            return -1;
        }

        var data = sock.recv(fd, readlength);
        if (data === null){
            if (process.errno === EWOULDBLOCK){
                continue;
            }

            if (process.errno === errno.EOF){
                break;
            } else {
                resultObject.errorMsg  = "Error reading data from crate server";
                resultObject.errorCode = process.errno;
                return -1;
            }
        }

        response += data || '';
        //FIXME this indicates a partial read, should we
        //check again until we get EOF?!
        if (data.length === 0 || data.length < readlength){
            break
        }
    }
    
    var res = response.split("\r\n\r\n");
    // print(res[1]);

    //wrap response parsing in try catch block
    //because we might get a malformed body
    //and parsing just failed
    var responseBody;
    try {
        responseBody = JSON.parse(res[1]);
    } catch (e){
        resultObject.errorMsg  = "Can't parse response body";
        resultObject.errorCode = 0;
        return -1;
    }

    //error
    if (responseBody.error){
        var errCode = responseBody.error.code;
        var errMessage = responseBody.error.message || '';
        var error = ERRORS[errCode] ? ERRORS[errCode] + " " : "";

        //extending error message with better explanation
        var extendedError = (errMessage.match(/\[(.*?)\]/))[0];
        // print(extendedError);

        resultObject.errorMsg = error + extendedError;
        resultObject.errorCode = errCode;
        return -1;
    }

    resultObject.headers = res[0];
    resultObject.raw     = res[1];
    resultObject.data    = JSON.parse(res[1]);
    resultObject.cols    = resultObject.data.cols || [];

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
