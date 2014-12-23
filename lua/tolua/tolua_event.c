/* tolua: event functions
** Support code for Lua bindings.
** Written by Waldemar Celes, modified by Jin Li
** TeCGraf/PUC-Rio
** Apr 2003, Apr 2014
** $Id: $
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#include <stdio.h>

#include "tolua++.h"

/* Store at ubox
	* It stores, creating the corresponding table if needed,
	* the pair key/value in the corresponding ubox table
	*/
static void storeatubox(lua_State* L, int lo)
{
	lua_getfenv(L, lo);
	if (lua_rawequal(L, -1, TOLUA_NOPEER))
	{
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setfenv(L, lo);/* stack: k,v,table  */
	}
	lua_insert(L, -3);
	lua_settable(L, -3);/* on lua 5.1, we trade the "tolua_peers" lookup for a settable call */
	lua_pop(L, 1);
}

/* Module index function
*/
static int module_index_event(lua_State* L)
{
	lua_rawgeti(L, -2, MT_GET);
	if (lua_istable(L, -1))
	{
		lua_pushvalue(L, 2);  /* key */
		lua_rawget(L, -2);
		if (lua_iscfunction(L, -1))
		{
			lua_call(L, 0, 1);
			return 1;
		}
		else if (lua_istable(L, -1)) return 1;
	}
	/* call old index meta event */
	if (lua_getmetatable(L, 1))
	{
		lua_pushstring(L, "__index");
		lua_rawget(L, -2);
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		if (lua_isfunction(L, -1))
		{
			lua_call(L, 2, 1);
			return 1;
		}
		else if (lua_istable(L, -1))
		{
			lua_gettable(L, -3);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

/* Module newindex function
*/
static int module_newindex_event(lua_State* L)
{
	lua_rawgeti(L, -3, MT_SET);
	if (lua_istable(L, -1))
	{
		lua_pushvalue(L, 2);  /* key */
		lua_rawget(L, -2);
		if (lua_iscfunction(L, -1))
		{
			lua_pushvalue(L, 1); /* only to be compatible with non-static vars */
			lua_pushvalue(L, 3); /* value */
			lua_call(L, 2, 0);
			return 0;
		}
	}
	/* call old newindex meta event */
	if (lua_getmetatable(L, 1) && lua_getmetatable(L, -1))
	{
		lua_pushstring(L, "__newindex");
		lua_rawget(L, -2);
		if (lua_isfunction(L, -1))
		{
			lua_pushvalue(L, 1);
			lua_pushvalue(L, 2);
			lua_pushvalue(L, 3);
			lua_call(L, 3, 0);
		}
	}
	lua_settop(L, 3);
	lua_rawset(L, -3);
	return 0;
}

/* Class index function
	* If the object is a userdata(ie, an object), it searches the field in
	* the alternative table stored in the corresponding "ubox" table.
	*/
static int class_index_event(lua_State* L)
{
	/* 1 ud��2 key */
	int t = lua_type(L, 1);
	if (t == LUA_TUSERDATA)// __index for ud
	{
		/* access peer table */
		lua_getfenv(L, 1);// peer
		if (!lua_rawequal(L, -1, TOLUA_NOPEER))
		{
			lua_pushvalue(L, 2);// peer key
			lua_gettable(L, -2);// peer[key], peer value
			if (!lua_isnil(L, -1))// value != nil
			{
				return 1;
			}
		}
		lua_settop(L, 2);// ud key
		lua_getmetatable(L, 1);// ud key mt
		/* try metatables */
		lua_pushvalue(L, 1);// ud key mt ud
		int loop = 0;
		while (lua_getmetatable(L, -1))
		{
			/* ud key mt ud mt */
			lua_remove(L, -2);// ud key mt mt
			if (lua_isnumber(L, 2))// check if key is a numeric value
			{
				/* try operator[] */
				lua_rawgeti(L, -1, MT_GETI);// ud key mt mt func
				if (lua_isfunction(L, -1))
				{
					if (loop)
					{
						lua_pushvalue(L, -1);
						lua_rawseti(L, 3, MT_GETI);
					}
					lua_pushvalue(L, 1);
					lua_pushvalue(L, 2);
					lua_call(L, 2, 1);
					return 1;
				}
			}
			else
			{
				lua_pushvalue(L, 2);// ud key mt mt key
				lua_rawget(L, -2);// mt[key], ud key mt mt value
				if (!lua_isnil(L, -1))
				{
					if (loop)
					{
						lua_pushvalue(L, 2); // ud key mt mt value key
						lua_pushvalue(L, -2); // ud key mt mt value key value
						lua_rawset(L, 3); // mt[key] = value, ud key mt mt value
					}
					return 1;
				}
				else lua_pop(L, 1);
				/* try C/C++ variable */
				lua_rawgeti(L, -1, MT_GET);// ud key mt mt tget
				if (lua_istable(L, -1))
				{
					lua_pushvalue(L, 2);
					lua_rawget(L, -2);// ud key mt mt value
					if (lua_iscfunction(L, -1))
					{
						if (loop)
						{
							lua_rawgeti(L, 3, MT_GET); // cfunc get
							if (lua_isnil(L, -1)) // get == nil
							{
								lua_pop(L, 1); // cfunc
								lua_newtable(L); // cfunc tb
								lua_pushvalue(L, -1); // cfunc tb tb
								lua_rawseti(L, 3, MT_GET); // mt[MT_GET] = tb, cfunc tb
							}
							lua_pushvalue(L, 2); // cfunc get key
							lua_pushvalue(L, -3); // cfunc get key cfunc
							lua_rawset(L, -3); // set[key] = cfunc, cfunc get
							lua_pop(L, 1); // cfunc
						}
						lua_pushvalue(L, 1);
						lua_pushvalue(L, 2);
						lua_call(L, 2, 1);
						return 1;
					}
					else if (lua_istable(L, -1))
					{
						/* deal with array: create table to be returned and cache it in ubox */
						void* u = *((void**)lua_touserdata(L, 1));
						lua_newtable(L);/* stack: obj key mt value table */
						lua_pushlightuserdata(L, u);
						lua_rawseti(L, -2, MT_SELF);/* store usertype at index MT_SELF */
						lua_insert(L, -2);/* stack: obj key mt table value */
						lua_setmetatable(L, -2);/* set stored value as metatable */
						lua_pushvalue(L, -1);/* stack: obj key met table table */
						lua_pushvalue(L, 2);/* stack: obj key mt table table key */
						lua_insert(L, -2);/*  stack: obj key mt table key table */
						storeatubox(L, 1);/* stack: obj key mt table */
						return 1;
					}
				}
			}
			lua_settop(L, 4);// obj key mt basemt
			loop++;
		}
		lua_pushnil(L);
		return 1;
	}
	else if (t == LUA_TTABLE)// __index for ud`s class
	{
		module_index_event(L);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

/* Newindex function
	* It first searches for a C/C++ varaible to be set.
	* Then, it either stores it in the alternative ubox table(in the case it is
	* an object) or in the own table(that represents the class or module).
	*/
static int class_newindex_event(lua_State* L)
{
	/* 1 ud, 2 key, 3 value */
	int t = lua_type(L, 1);
	if (t == LUA_TUSERDATA)// __newindex for ud
	{
		/* access peer table first */
		lua_getfenv(L, 1);// peer
		if (!lua_rawequal(L, -1, TOLUA_NOPEER))
		{
			lua_pushvalue(L, 2);// peer key
			lua_gettable(L, -2);// peer[key], peer value
			if (!lua_isnil(L, -1))
			{
				lua_pop(L, 1);// peer
				lua_pushvalue(L, 2); // peer key
				lua_pushvalue(L, 3); // peer key value
				lua_settable(L, -3); // peer[key] = value, peer
				lua_pop(L, 1);// local empty
				return 1;
			}
		}
		lua_settop(L, 3);// ud key value
		int loop = 0;
		lua_getmetatable(L, 1); // ud key value mt
		lua_pushvalue(L, -1); // ud key value mt mt
		while (lua_istable(L, -1)) // mt is table
		{
			if (lua_isnumber(L, 2)) // key is number
			{
				/* try operator[] */
				lua_rawgeti(L, -1, MT_SETI); // mt[MT_SETI], ud key value mt seti 
				if (lua_isfunction(L, -1)) // seti is function
				{
					if (loop)
					{
						lua_pushvalue(L, -1);
						lua_rawseti(L, 4, MT_SETI);
					}
					lua_pushvalue(L, 1); // ud key value mt seti ud
					lua_pushvalue(L, 2); // ud key value mt seti ud key
					lua_pushvalue(L, 3); // ud key value mt seti ud key value
					lua_call(L, 3, 0); // ud key value mt
					return 0;
				}
			}
			else
			{
				lua_rawgeti(L, -1, MT_SET);
				if (lua_istable(L, -1))
				{
					lua_pushvalue(L, 2);
					lua_rawget(L, -2);                     /* stack: t k v mt tset func */
					if (lua_iscfunction(L, -1))
					{
						if (loop)
						{
							lua_rawgeti(L, 4, MT_SET); // cfunc set
							if (lua_isnil(L, -1)) // set == nil
							{
								lua_pop(L, 1); // cfunc
								lua_newtable(L); // cfunc tb
								lua_pushvalue(L, -1); // cfunc tb tb
								lua_rawseti(L, 4, MT_SET); // mt[MT_SET] = tb, cfunc tb
							}
							lua_pushvalue(L, 2); // cfunc set key
							lua_pushvalue(L, -3); // cfunc set key cfunc
							lua_rawset(L, -3); // set[key] = cfunc, cfunc set
							lua_pop(L, 1); // cfunc
						}
						lua_pushvalue(L, 1);
						lua_pushvalue(L, 3);
						lua_call(L, 2, 0);
						return 0;
					}
					lua_pop(L, 1);                          /* stack: t k v mt tset */
				}
				lua_pop(L, 1);/* stack: t k v mt */

				if (!lua_getmetatable(L, -1))/* stack: t k v mt mt */
				{
					lua_pushnil(L);
				}
				lua_remove(L, -2);/* stack: t k v mt */
				loop++;
			}
		}
		lua_settop(L, 3);/* stack: t k v */

		/* then, store as a new field */
		storeatubox(L, 1);
	}
	else if (t == LUA_TTABLE)// __newindex for ud`s class
	{
		module_newindex_event(L);
	}
	return 0;
}

static int class_call_event(lua_State* L)
{
	if (lua_istable(L, 1))
	{
		lua_rawgeti(L, 1, MT_CALL);
		if (lua_isfunction(L, -1))
		{
			lua_insert(L, 1);
			lua_call(L, lua_gettop(L) - 1, 1);
			return 1;
		};
	};
	tolua_error(L, "Attempt to call a non-callable object.", NULL);
	return 0;
}

static int do_operator(lua_State* L, int op)
{
	if (lua_isuserdata(L, 1))
	{
		/* Try metatables */
		lua_pushvalue(L, 1);                     /* stack: op1 op2 */
		while (lua_getmetatable(L, -1))
		{   /* stack: op1 op2 op1 mt */
			lua_remove(L, -2);                      /* stack: op1 op2 mt */
			lua_rawgeti(L, -1, op);                      /* stack: obj key mt func */
			if (lua_isfunction(L, -1))
			{
				lua_pushvalue(L, 1);
				lua_pushvalue(L, 2);
				lua_call(L, 2, 1);
				return 1;
			}
			lua_settop(L, 3);
		}
	}
	tolua_error(L, "Attempt to perform operation on an invalid operand", NULL);
	return 0;
}

static int class_add_event(lua_State* L)
{
	return do_operator(L, MT_ADD);
}

static int class_sub_event(lua_State* L)
{
	return do_operator(L, MT_SUB);
}

static int class_mul_event(lua_State* L)
{
	return do_operator(L, MT_MUL);
}

static int class_div_event(lua_State* L)
{
	return do_operator(L, MT_DIV);
}

static int class_lt_event(lua_State* L)
{
	return do_operator(L, MT_LT);
}

static int class_le_event(lua_State* L)
{
	return do_operator(L, MT_LE);
}

static int class_eq_event(lua_State* L)
{
	/* copying code from do_operator here to return false when no operator is found */
	if (lua_isuserdata(L, 1))
	{
		/* Try metatables */
		lua_pushvalue(L, 1);                     /* stack: op1 op2 */
		while (lua_getmetatable(L, -1))
		{   /* stack: op1 op2 op1 mt */
			lua_remove(L, -2);                      /* stack: op1 op2 mt */
			lua_rawgeti(L, -1, MT_EQ);                      /* stack: obj key mt func */
			if (lua_isfunction(L, -1))
			{
				lua_pushvalue(L, 1);
				lua_pushvalue(L, 2);
				lua_call(L, 2, 1);
				return 1;
			}
			lua_settop(L, 3);
		}
	}
	lua_settop(L, 3);
	lua_pushboolean(L, 0);
	return 1;
}

TOLUA_API int class_gc_event(lua_State* L)
{
	lua_getmetatable(L, 1);
	lua_rawgeti(L, -1, MT_DEL); // stack: mt collector
	if (lua_isfunction(L, -1))
	{
		lua_pushvalue(L, 1); // stack: mt collector u
		lua_call(L, 1, 0);
	}
#ifdef __SHOW_NO_GC_COLLECTOR_MSG__
	else
	{
		char* typeName;
		lua_pop(L, 1);
		lua_rawget(L, LUA_REGISTRYINDEX);  /* get registry[mt] */
		typeName = lua_tostring(L, -1);
		luaL_error(L, "No gc collector founded in %s", typeName);
	}
#endif
	lua_pop(L, 1);
	return 0;
}

/* Register module events
	* It expects the metatable on the top of the stack
	*/
TOLUA_API void tolua_moduleevents(lua_State* L)
{
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, module_index_event);
	lua_rawset(L, -3);
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, module_newindex_event);
	lua_rawset(L, -3);
}

/* Check if the object on the top has a module metatable
*/
TOLUA_API int tolua_ismodulemetatable(lua_State* L)
{
	int r = 0;
	if (lua_getmetatable(L, -1))
	{
		lua_pushstring(L, "__index");
		lua_rawget(L, -2);
		r = (lua_tocfunction(L, -1) == module_index_event);
		lua_pop(L, 2);
	}
	return r;
}

/* Register class events
	* It expects the metatable on the top of the stack
	*/
TOLUA_API void tolua_classevents(lua_State* L)
{
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, class_index_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, class_newindex_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, class_add_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, class_sub_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, class_mul_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, class_div_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__lt");
	lua_pushcfunction(L, class_lt_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__le");
	lua_pushcfunction(L, class_le_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, class_eq_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__call");
	lua_pushcfunction(L, class_call_event);
	lua_rawset(L, -3);

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, class_gc_event);
	lua_rawset(L, -3);
}
