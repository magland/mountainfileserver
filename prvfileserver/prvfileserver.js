console.log('Running prvfileserver...');

//// requires
var	url=require('url');
var http=require('http');
var fs=require('fs');

//use > npm install extend
var extend=require('extend');

var config=JSON.parse(fs.readFileSync(__dirname+'/../prvfileserver.json.default','utf8'));
try {
	config_user=ini.parse(fs.readFileSync(__dirname+'/../config/prvfileserver.json','utf8'));
	config=extend(true,config,config_user);
}
catch(err) {
}

var subservers=config.subservers||[];

process.on('SIGINT', function() {
    process.exit();
});

console.log('CONFIG:');
console.log(config);
console.log('');

console.log('SUBSERVERS:');
console.log(subservers);
console.log('');

http.createServer(function (REQ, RESP) {
	var url_parts = url.parse(REQ.url,true);	
	if (REQ.method == 'OPTIONS') {
		var headers = {};
		
		//allow cross-domain requests
		/// TODO refine this
		
		headers["Access-Control-Allow-Origin"] = "*";
		headers["Access-Control-Allow-Methods"] = "POST, GET, PUT, DELETE, OPTIONS";
		headers["Access-Control-Allow-Credentials"] = false;
		headers["Access-Control-Max-Age"] = '86400'; // 24 hours
		headers["Access-Control-Allow-Headers"] = "X-Requested-With, X-HTTP-Method-Override, Content-Type, Accept";
		RESP.writeHead(200, headers);
		RESP.end();
	}
	else if (REQ.method=='GET') {
		console.log('GET: '+REQ.url);
		var path=url_parts.pathname;
		var query=url_parts.query;
		var method=query.a||'download';

		var recursion_index=0;
		if (is_an_integer_between(Number(query.recursion_index),0,1000)) {
			recursion_index=Number(query.recursion_index);
		}
		else {
			recursion_index=Number(config.max_recursion||5);
			if (!is_an_integer_between(recursion_index,0,1000)) {
				send_text_response('Unexpected problem with recursion_index: '+recursion_index);
				return;
			}
		}
		console.log('::::::::::::::::::::::::::::::::::::::: '+recursion_index);


		if (recursion_index<=0) {
			send_text_response('WARNING: max recursion reached. You probably have cyclic dependencies!!');
			return;
		}

		if (method=="download") {
			var fname=absolute_data_directory()+"/"+path;
			if (!require('fs').existsSync(fname)) {
				send_json_response({success:false,error:"File does not exist: "+path});		
				return;	
			}
			serve_file(fname,RESP);
		}
		else if (method=="stat") {
			var fname=absolute_data_directory()+"/"+path;
			if (!require('fs').existsSync(fname)) {
				send_json_response({success:false,error:"File does not exist: "+path});		
				return;	
			}
			run_process_and_read_stdout(__dirname+'/../bin/sumit',['stat',fname],function(txt) {
				try {
					var obj=JSON.parse(txt);
					send_json_response(obj);
				}
				catch(err) {
					send_json_response({success:false,error:'Problem parsing json response from sumit stat: '+txt});
				}
			});
		}
		else if (method=="find") {
			if ((!query.checksum)||(!query.size)) {
				send_json_response({success:false,error:"Invalid query."});	
				return;
			}
			run_process_and_read_stdout(__dirname+'/../bin/sumit',['find',absolute_data_directory(),'--checksum='+query.checksum,'--size='+query.size],function(txt) {
				txt=txt.trim();
				if (!txt) {
					find_in_subserver({checksum:query.checksum,size:query.size});
					return;
				}
				if (txt) txt=txt.slice(absolute_data_directory().length+1);
				send_as_text_or_link(txt);
			});	
		}
		else {
			send_json_response({success:false,error:"Unrecognized method: "+method});
		}
	}
	else if(REQ.method=='POST') {
		send_text_response("POST not supported!");
	}
	else {
		send_text_response("Unsuported request method.");
	}

	function find_in_subserver(info) {
		foreach_async(subservers,find_in_subserver2,function(txt) {
			send_as_text_or_link(txt);
		});
		function find_in_subserver2(subserver0,callback) {
			var url0=subserver0.host+':'+subserver0.port+subserver0.path+'?a=find&checksum='+info.checksum+'&size='+info.size+'&recursion_index='+(Number(recursion_index)-1);
			http_get_text_file(url0,function(txt0) {
				if (txt0) {
					var txt1=txt0;
					if (looks_like_it_could_be_a_file_path(txt0))
						txt1=subserver0.host+':'+subserver0.port+'/'+txt0;
					callback({done:true,result:txt1});
				}
				else {
					callback({done:false});
				}
			});
		}
	}

	function foreach_async(list,func,callback) {
		var index=0;
		do_next();
		function do_next() {
			if (index>=list.length) {
				callback(null);
				return;
			}
			func(list[index],function(ret) {
				if (ret.done) {
					callback(ret.result);
				}
				else {
					index++;
					do_next();
				}
			});
		}
	}

	function http_get_text_file(url,callback) {	
		http.get(url, function(res) {
			res.setEncoding('utf8'); //important!

			var txt='';
			res.on("data", function(chunk) {
				txt+=chunk;
			});
			res.on('end',function() {
				callback(txt);
			});
		}).on('error', function(e) {
			console.log("Error in response from "+url+": " + e.message);
			callback('');
		});
	}

	function absolute_data_directory() {
		var ret=config.data_directory;
		if (ret.indexOf('/')===0) return ret;
		return __dirname+'/../'+ret;
	}
	
	function send_json_response(obj) {
		RESP.writeHead(200, {"Access-Control-Allow-Origin":"*", "Content-Type":"application/json"});
		RESP.end(JSON.stringify(obj));
	}
	function send_text_response(text) {
		RESP.writeHead(200, {"Access-Control-Allow-Origin":"*", "Content-Type":"text/plain"});
		RESP.end(text);
	}
	function send_html_response(text) {
		RESP.writeHead(200, {"Access-Control-Allow-Origin":"*", "Content-Type":"text/html"});
		RESP.end(text);
	}
	function send_as_text_or_link(text) {
		if ((query.return_link=='true')&&(looks_like_it_could_be_a_file_path(text))) {
			if (text) {
				send_html_response('<html><body><a href="'+text+'">'+text+'</a></body></html>');
			}
			else {
				send_html_response('<html><body>Not found.</html>');	
			}
		}
		else {
			send_text_response(text);
		}
	}
}).listen(config.listen_port);
console.log ('Listening on port '+config.listen_port);

function run_process_and_read_stdout(exe,args,callback) {
	console.log('RUNNING:'+exe+' '+args.join(' '));
	var P=require('child_process').spawn(exe,args);
	var txt='';
	P.stdout.on('data',function(chunk) {
		txt+=chunk;
	});
	P.on('close',function(code) {
		callback(txt);
	});
}

function serve_file(filename,response) {
	response.writeHead(200, {"Access-Control-Allow-Origin":"*", "Content-Type":"application/json"});
	fs.exists(filename,function(exists) {
		if (!exists) {
			response.writeHead(404, {"Content-Type": "text/plain"});
			response.write("404 Not Found\n");
			response.end();
			return;
		}

		fs.readFile(filename, "binary", function(err, file) {
			if(err) {        
				response.writeHead(500, {"Content-Type": "text/plain"});
				response.write(err + "\n");
				response.end();
				return;
			}

			//response.writeHead(200);
			response.write(file, "binary");
			response.end();
		});
	});
}

//is this used?
function mkdir_if_needed(path) {
	var fs=require('fs');
	if (!fs.existsSync(path)){
    	fs.mkdirSync(path);
	}
}

function is_an_integer_between(num,i1,i2) {
	for (var i=i1; i<=i2; i++) {
		if (num===i) return true;
	}
	return false;
}

function looks_like_it_could_be_a_file_path(txt) {
	if (txt.indexOf(' ')>=0) return false;
	return true;
}
