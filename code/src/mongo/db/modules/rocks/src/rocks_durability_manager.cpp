/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include "mongo/db/storage/journal_listener.h"

#include <rocksdb/db.h>

#include "rocks_durability_manager.h"
#include "rocks_util.h"
#include "mongo/util/log.h"

namespace mongo {
    RocksDurabilityManager::RocksDurabilityManager(rocksdb::DB* db, bool durable, 
        const std::vector<rocksdb::ColumnFamilyHandle*>& cfHandles)
        : _db(db), _durable(durable), _journalListener(&NoOpJournalListener::instance)
        ,_columnFamilyHandles(cfHandles){}


    void RocksDurabilityManager::setJournalListener(JournalListener* jl) {
        stdx::unique_lock<stdx::mutex> lk(_journalListenerMutex);
        _journalListener = jl;
    }

    void RocksDurabilityManager::waitUntilDurable(bool forceFlush) {
        stdx::unique_lock<stdx::mutex> lk(_journalListenerMutex);
        JournalListener::Token token;
        if (_journalListener) {
            token = _journalListener->getToken();
        }

        if (!_durable || forceFlush) {
            invariantRocksOK(_db->Flush(rocksdb::FlushOptions()));
        } else {
            invariantRocksOK(_db->SyncWAL());
        }

        if (_journalListener) {
            _journalListener->onDurable(token);
        }
    }

    //rocksdb::Status RocksDurabilityManager::flushMemTable(void) {
    //    rocksdb::FlushOptions option;
    //    option.wait = false;
    //    for (auto cfHandle : _columnFamilyHandles)
    //    {
    //         auto status = _db->Flush(option, cfHandle);
    //         if (!status.ok()) {
    //             index_err() << "MaybeFlush fail: " << status.ToString();
    //         }
    //    }

    //    return  rocksdb::Status::OK();
    //}

}  // namespace mongo