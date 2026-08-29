/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef RENDERWORKERPOOL_H
#define RENDERWORKERPOOL_H

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <deque>
#include <memory>

#include "codec/frame.h"
#include "node/project/serializer/serializer.h"
#include "render/ipc/frameslotpool.h"
#include "render/ipc/ipcmessage.h"
#include "render/ipc/sharedmemoryregion.h"
#include "render/rendermanager.h"

class QProcess;

namespace olive
{

class Project;

class RenderWorkerPool : public QThread {
	Q_OBJECT
public:
	explicit RenderWorkerPool(DecoderCache *decoder_cache,
							  const QString &gpu_backend,
							  QObject *parent = nullptr);
	~RenderWorkerPool() override;

	bool SubmitFrame(RenderTicketPtr ticket,
					 const RenderManager::RenderVideoParams &params);

	bool RemoveTicket(RenderTicketPtr ticket);

	void Shutdown();

protected:
	void run() override;

private:
	struct Job {
		Job(RenderTicketPtr t, const RenderManager::RenderVideoParams &p)
			: ticket(t)
			, params(p)
		{
		}

		RenderTicketPtr ticket;
		RenderManager::RenderVideoParams params;
		QString graph_path;
		QString node_token;
		QVector<FramePtr> input_frames;
	};

	enum class JobResult {
		kFinished,
		kRetryableFailure,
		kFatalFailure,
		kCancelled
	};

	struct ActiveJob {
		RenderTicketPtr ticket;
		qint64 process_id = 0;
		qint64 ticket_id = 0;
	};

	struct PooledWorker {
		QProcess *process = nullptr;
		QString loaded_graph_path;
		qint64 last_used_ms = 0;
		int use_count = 0;

		// Persistent shared memory for this worker. Reusing regions across frames
		// avoids the cost of creating/destroying large shm segments every render.
		ipc::SharedMemoryRegion output_region;
		ipc::FrameSlotPool output_pool;
		size_t output_slot_bytes = 0;
		QString output_shm_key;

		ipc::SharedMemoryRegion input_region;
		ipc::FrameSlotPool input_pool;
		size_t input_slot_bytes = 0;
		QString input_shm_key;
	};

	struct CachedGraph {
		QString path;
	};

	bool PrepareJob(RenderTicketPtr ticket,
					const RenderManager::RenderVideoParams &params, Job *job);
	bool WriteGraphSnapshot(Project *project, QString *path);
	bool IsSupported(const RenderManager::RenderVideoParams &params) const;

	void WorkerLoop(int worker_index,
					std::vector<std::unique_ptr<PooledWorker>> *local_pool);
	void ProcessJob(const Job &job, int worker_index,
					std::vector<std::unique_ptr<PooledWorker>> *local_pool);
	JobResult ProcessJobAttempt(const Job &job, int worker_index,
								int attempt_index, PooledWorker *worker);
	void FinishWithFrame(RenderTicketPtr ticket, const ipc::FrameSlotPool &pool,
						 uint32_t slot);
	void CleanupGraphFile(const QString &path);
	void AddGraphPathRef(const QString &path);
	void AddGraphPathRefLocked(const QString &path);
	void ReleaseGraphPathRef(const QString &path);
	void ReleaseGraphPathRefLocked(const QString &path);
	void SetGraphPathCached(const QString &path, bool cached);
	void SetGraphPathCachedLocked(const QString &path, bool cached);
	void CancelActiveProcess(qint64 process_id);
	void SetActiveWorker(int worker_index, RenderTicketPtr ticket,
						 QProcess *worker, qint64 ticket_id);
	void ClearActiveWorker(int worker_index, qint64 process_id);
	int WorkerCount() const;

	std::unique_ptr<PooledWorker>
	AcquireWorker(std::vector<std::unique_ptr<PooledWorker>> *local_pool,
				  const QString &graph_path);
	void ReturnWorker(std::vector<std::unique_ptr<PooledWorker>> *local_pool,
					  std::unique_ptr<PooledWorker> worker, bool keep_alive);
	void ShutdownWorker(PooledWorker *worker);
	void
	ShutdownLocalPool(std::vector<std::unique_ptr<PooledWorker>> *local_pool);
	void ClearGraphCache();

	DecoderCache *decoder_cache_;
	QString gpu_backend_;
	QMutex mutex_;
	QWaitCondition wait_;
	std::deque<Job> queue_;
	bool stopping_ = false;
	QVector<ActiveJob> active_jobs_;
	QHash<QUuid, CachedGraph> graph_cache_;
	QHash<QString, int> graph_path_ref_count_;
	QSet<QString> cached_graph_paths_;

	static constexpr uint32_t kOutputSlots = 2;
	static constexpr int kMaxAttempts = 2;
	static constexpr int kMaxWidth = 4096;
	static constexpr int kMaxHeight = 2160;
	static constexpr int kWorkerIdleTimeoutMs = 30000;
	static constexpr int kWorkerMaxUses = 100;
};

}

#endif // RENDERWORKERPOOL_H
