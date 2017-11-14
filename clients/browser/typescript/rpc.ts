module RPC {

	interface Message {
		jsonrpc: string;
		id: number;
	}

	interface MessageRequest extends Message {
		method: string;
		params?: any
	}

	interface MessageError extends Message {
		error: {
			code: number;
			message: string;
		}
	}

	// Utility
	function isUndefined(value: any): boolean { return value===undefined; }

	let isArray=Array.isArray;

	function isObject(value: any): boolean {
		let type=typeof value;
		return value!=null && (type==='object' || type==='function');
	}

	function isFunction(value: any): boolean { return typeof value==='function'; }

	function isString(value: any): boolean { return typeof value==='string'; }

	function isEmpty(value: any): boolean {
		if(isObject(value)){
			for(let idx in value){
				if(value.hasOwnProperty(idx)) return false;
			}
			return true;
		}
		else if(isArray(value)) return !value.length;
		return !value;
	}

	function forEach(target: any, callback: any){
		if(isArray(target)) return target.map(callback);
		for(let key in target){
			if(target.hasOwnProperty(key)) callback(target[key]);
		}
	}

	function clone(value: object): object { return JSON.parse(JSON.stringify(value)); }

	// RPC Class
	export class RPC {
		static ERRORS: {
	        "PARSE_ERROR": {
	            "code": -32700,
	            "message": "Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text."
	        },
	        "INVALID_REQUEST": {
	            "code": -32600,
	            "message": "Invalid Request. The JSON sent is not a valid Request object."
	        },
	        "METHOD_NOT_FOUND": {
	            "code": -32601,
	            "message": "Method not found. The method does not exist / is not available."
	        },
	        "INVALID_PARAMS": {
	            "code": -32602,
	            "message": "Invalid params. Invalid method parameter(s)."
	        },
	        "INTERNAL_ERROR": {
	            "code": -32603,
	            "message": "Internal error. Internal JSON-RPC error."
	        }
	    };

	    ServerError=class extends Error {
	    	code: number;
	    	data: object;

	    	constructor(code: number, message: string, data?: any){
	    		super();
	    		this.message=message || "";
	    		this.code=code || -32000;
	    		if(Boolean(data)) this.data=data;
	    	}
	    }

		_address: string;
		_socket: WebSocket;

		_waitingFrame: object={}
		_id: number=0;
		_dispatcher: object={}

		constructor(){}

		_setError(rpcError, exception?){
			let error=clone(rpcError);

            if(!!exception){
                if(isObject(exception) && exception.hasOwnProperty("message")){
                    error['data']=exception.message;
                }
                else if(isString(exception)){
                    error['data']=exception;
                }

                if(exception instanceof this.ServerError){
                    error={
                        message: exception.message,
                        code: exception.code
                    };
                    if(exception.hasOwnProperty('data')){
                        error['data']=exception.data;
                    }
                }
            }
            return error;
        }

        _isPromise(thing){ return !!thing && 'function'===typeof thing.then; }

        _isError(message){ return !!message.error; }

        _isRequest(message){ return !!message.method; }

        _isResponse(message){ return message.hasOwnProperty('result') && message.hasOwnProperty('id'); }

        _beforeResolve(message){
            var promises=[];
            if(isArray(message)){
                forEach(message, function (msg){
                    promises.push(this._resolver(msg));
                });
            }
            else if(isObject(message)) promises.push(this._resolver(message));

            return Promise.all(promises)
                .then(function (result){
                    var toStream=[];
                    forEach(result, function (r){
                        if(!isUndefined(r)){
                            toStream.push(r);
                        }
                    });

                    if(toStream.length===1){
                        this.toStream(JSON.stringify(toStream[0]));
                    }
                    else if(toStream.length > 1){
                        this.toStream(JSON.stringify(toStream));
                    }
                    return result;
                });
        }

        _resolver(message){
            try {
                if(this._isError(message)) return this._rejectRequest(message);
                else if(this._isResponse(message)) return this._resolveRequest(message);
                else if(this._isRequest(message)) return this._handleRemoteRequest(message);
                else {
                    return Promise.resolve({
                        "id": null,
                        "jsonrpc": "2.0",
                        "error": this._setError(RPC.ERRORS.INVALID_REQUEST)
                    });
                }
            }
            catch (e){
                console.error('Resolver error:'+e.message, e);
                return Promise.reject(e);
            }
        }

        _rejectRequest(error){
            if(this._waitingFrame.hasOwnProperty(error.id)){
                this._waitingFrame[error.id].reject(error.error);
            }
            else {
                console.log('Unknown request', error);
            }
        }

        _resolveRequest(result){
            if(this._waitingFrame.hasOwnProperty(result.id)){
                this._waitingFrame[result.id].resolve(result.result);
                delete this._waitingFrame[result.id];
            }
            else {
                console.log('unknown request', result);
            }
        }

        _handleRemoteRequest(request){
            if(this._dispatcher.hasOwnProperty(request.method)){
                try {
                    var result;

                    if(request.hasOwnProperty('params')){
                        if(this._dispatcher[request.method].params == "pass")
                            result=this._dispatcher[request.method].fn.call(this._dispatcher, request.params);
                        else if(isArray(request.params))
                            result=this._dispatcher[request.method].fn.apply(this._dispatcher, request.params);
                        else if(isObject(request.params)){
                            if(this._dispatcher[request.method].params instanceof Array){
                                var argsValues=[];
                                this._dispatcher[request.method].params.forEach(function (arg){

                                    if(request.params.hasOwnProperty(arg)){
                                        argsValues.push(request.params[arg]);
                                        delete request.params[arg];
                                    }
                                    else {
                                        argsValues.push(undefined);
                                    }
                                });

                                if(Object.keys(request.params).length > 0){
                                    return Promise.resolve({
                                        "jsonrpc": "2.0",
                                        "id": request.id,
                                        "error": this._setError(RPC.ERRORS.INVALID_PARAMS, {
                                            message: "Params: "+Object.keys(request.params).toString()+" not used"
                                        })
                                    });
                                }
                                else {
                                    result=this._dispatcher[request.method].fn.apply(this._dispatcher, argsValues);
                                }
                            }
                            else {
                                return Promise.resolve({
                                    "jsonrpc": "2.0",
                                    "id": request.id,
                                    "error": this._setError(RPC.ERRORS.INVALID_PARAMS, "Undeclared arguments of the method "+request.method)
                                });
                            }
                        } else result=this._dispatcher[request.method].fn.call(this._dispatcher, request.params);
                    }
                    else result=this._dispatcher[request.method].fn();

                    if(request.hasOwnProperty('id')){
                        if(this._isPromise(result)){
                            return result.then(function (res){
                                if(isUndefined(res)){
                                    res=true;
                                }
                                return {
                                    "jsonrpc": "2.0",
                                    "id": request.id,
                                    "result": res
                                };
                            })
                                .catch(function (e){
                                    return {
                                        "jsonrpc": "2.0",
                                        "id": request.id,
                                        "error": this._setError(RPC.ERRORS.INTERNAL_ERROR, e)
                                    };
                                });
                        }
                        else {

                            if(isUndefined(result)){
                                result=true;
                            }

                            return Promise.resolve({
                                "jsonrpc": "2.0",
                                "id": request.id,
                                "result": result
                            });
                        }
                    }
                    else {
                        return Promise.resolve(); //nothing, it notification
                    }
                }
                catch (e){
                    return Promise.resolve({
                        "jsonrpc": "2.0",
                        "id": request.id,
                        "error": this._setError(RPC.ERRORS.INTERNAL_ERROR, e)
                    });
                }
            }
            else {
                return Promise.resolve({
                    "jsonrpc": "2.0",
                    "id": request.id,
                    "error": this._setError(RPC.ERRORS.METHOD_NOT_FOUND, {
                        message: request.method
                    })
                });
            }
        }

        _notification(method, params){
            var message={
                "jsonrpc": "2.0",
                "method": method,
                "params": params
            };

            if(isObject(params) && !isEmpty(params)){
                message.params=params;
            }

            return message;
        }

        _call(method, params){
            this._id+=1;
            var message={
                "jsonrpc": "2.0",
                "method": method,
                "id": this._id
            };

            //if(isObject(params) && !isEmpty(params))
                message['params']=params;

            return {
                promise: new Promise((resolve, reject)=>{

                    this._waitingFrame[this._id.toString()]={
                        resolve: resolve,
                        reject: reject
                    };
                }),
                message: message
            };
        }

        toStream(a){
            console.log('Need define the toStream method before use');
            console.log(arguments);
        };

        dispatch(functionName, paramsNameFn, fn){

            if(isString(functionName) && paramsNameFn == "pass" && isFunction(fn)){
                this._dispatcher[functionName]={
                    fn: fn,
                    params: paramsNameFn
                };
            }
            else if(isString(functionName) && isArray(paramsNameFn) && isFunction(fn)){
                this._dispatcher[functionName]={
                    fn: fn,
                    params: paramsNameFn
                };
            }
            else if(isString(functionName) && isFunction(paramsNameFn) && isUndefined(fn)){
                this._dispatcher[functionName]={
                    fn: paramsNameFn,
                    params: null
                };
            }
            else {
                throw new Error('Missing required argument: functionName - string, paramsNameFn - string or function');
            }
        };

        on=this.dispatch;

        off(functionName){ delete this._dispatcher[functionName]; };

        call(method, params){
            var _call=this._call(method, params);
            this.toStream(JSON.stringify(_call.message));
            return _call.promise;
        };

        notification(method, params){
            this.toStream(JSON.stringify(this._notification(method, params)));
        };

        batch(requests){
            var promises=[];
            var message=[];

            forEach(requests, function (req){
                if(req.hasOwnProperty('call')){
                    var _call=this.call(req.call.method, req.call.params);
                    message.push(_call.message);
                    promises.push(_call.promise.then(function (res){
                        return res;
                    }, function (err){
                        return err;
                    }));
                }
                else if(req.hasOwnProperty('notification')){
                    message.push(this.notification(req.notification.method, req.notification.params));
                }
            });

            this.toStream(JSON.stringify(message));
            return Promise.all(promises);
        };

        messageHandler(rawMessage){
            try {
                var message=JSON.parse(rawMessage);
                return this._beforeResolve(message);
            }
            catch (e){
                console.log("Error in messageHandler(): ", e);
                this.toStream(JSON.stringify({
                    "id": null,
                    "jsonrpc": "2.0",
                    "error": RPC.ERRORS.PARSE_ERROR
                }));
                return Promise.reject(e);
            }
        };

        customException(code, message, data){
            return new this.ServerError(code, message, data);
        };
	}
}