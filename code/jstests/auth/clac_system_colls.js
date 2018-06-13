/**
 * This tests that CLAC (collection level access control) handles system collections properly.
 */

// Verify that system collections are treated correctly
function runTest(admindb) {
    var authzErrorCode = 13;

    admindb.createUser({user: "admin", pwd: "Github@12", roles: ["userAdminAnyDatabase"], "passwordDigestor" : "server"});
    assert.eq(1, admindb.auth("admin", "Github@12"));

    var sysCollections = [
        "system.indexes",
        "system.js",
        "system.namespaces",
        "system.profile",
        "system.roles",
        "system.users"
    ];
    var sysPrivs = new Array();
    for (var i in sysCollections) {
        sysPrivs.push(
            {resource: {db: admindb.getName(), collection: sysCollections[i]}, actions: ['find']});
    }

    var findPriv = {
        resource: {db: admindb.getName(), collection: ""},
        actions: ['find']
    };

    admindb.createRole({role: "FindInDB", roles: [], privileges: [findPriv]});
    admindb.createRole({role: "FindOnSysRes", roles: [], privileges: sysPrivs});

    admindb.createUser({user: "sysUser", pwd: "Github@12", roles: ["FindOnSysRes"], "passwordDigestor" : "server"});
    admindb.createUser({user: "user", pwd: "Github@12", roles: ["FindInDB"], "passwordDigestor" : "server"});

    // Verify the find on all collections exludes system collections
    assert.eq(1, admindb.auth("user", "Github@12"));

    assert.doesNotThrow(function() {
        admindb.foo.findOne();
    });
    for (var i in sysCollections) {
        assert.commandFailed(admindb.runCommand({count: sysCollections[i]}));
    }

    // Verify that find on system collections gives find permissions
    assert.eq(1, admindb.auth("sysUser", "Github@12"));

    assert.throws(function() {
        admindb.foo.findOne();
    });
    for (var i in sysCollections) {
        assert.commandWorked(admindb.runCommand({count: sysCollections[i]}));
    }

    admindb.logout();
}

jsTest.log('Test standalone');
var conn = MongoRunner.runMongod({auth: ''});
runTest(conn.getDB("admin"));
MongoRunner.stopMongod(conn.port);

jsTest.log('Test sharding');
var st = new ShardingTest({shards: 2, config: 3, keyFile: 'jstests/libs/key1'});
runTest(st.s.getDB("admin"));
st.stop();
