#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QtTest/QSignalSpy>

#include "render/renderticket.h"

using namespace olive;

TEST(RenderTicketWatcher, DoesNotEmitFinishedForRunningTicketSynchronously)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	ticket->Start();

	RenderTicketWatcher watcher;
	QSignalSpy spy(&watcher, &RenderTicketWatcher::Finished);

	watcher.SetTicket(ticket);

	// The ticket is still running, so the watcher must not emit Finished
	// synchronously when SetTicket is called.
	EXPECT_EQ(spy.count(), 0);

	ticket->Finish();

	// Once the ticket finishes, the watcher should emit Finished.
	spy.wait(100);
	EXPECT_EQ(spy.count(), 1);
}

TEST(RenderTicketWatcher, EmitsFinishedForAlreadyFinishedTicketAsynchronously)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	ticket->Start();
	ticket->Finish();

	RenderTicketWatcher watcher;
	QSignalSpy spy(&watcher, &RenderTicketWatcher::Finished);

	watcher.SetTicket(ticket);

	// The ticket has already finished. The watcher must not delete itself or
	// emit Finished synchronously inside SetTicket, because the caller may still
	// need the returned pointer. Instead it should defer the signal.
	EXPECT_EQ(spy.count(), 0);
	EXPECT_FALSE(watcher.GetTicket() == nullptr);

	// Process the queued Finished emission.
	QCoreApplication::processEvents();

	EXPECT_EQ(spy.count(), 1);
}

TEST(RenderTicketWatcher, CancelMarksTicketAsCancelled)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	ticket->Start();

	RenderTicketWatcher watcher;
	watcher.SetTicket(ticket);

	EXPECT_TRUE(watcher.IsRunning());
	EXPECT_FALSE(ticket->IsCancelled());

	watcher.Cancel();

	EXPECT_TRUE(ticket->IsCancelled());
}

TEST(RenderTicket, HasResultIsFalseWhileRunning)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	ticket->Start();

	EXPECT_TRUE(ticket->IsRunning());
	EXPECT_FALSE(ticket->HasResult());
}

TEST(RenderTicket, FinishWithValueProvidesResult)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	ticket->Start();
	ticket->Finish(QVariant(42));

	EXPECT_FALSE(ticket->IsRunning());
	EXPECT_TRUE(ticket->HasResult());
	EXPECT_EQ(ticket->Get().toInt(), 42);
}

TEST(RenderTicket, FinishCountIncrementsOnEachFinish)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	EXPECT_EQ(ticket->GetFinishCount(), 0);

	ticket->Start();
	ticket->Finish();
	EXPECT_EQ(ticket->GetFinishCount(), 1);

	ticket->Start();
	ticket->Finish();
	EXPECT_EQ(ticket->GetFinishCount(), 2);
}

TEST(RenderTicket, FinishWithoutStartIsIgnored)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	ticket->Finish();
	EXPECT_EQ(ticket->GetFinishCount(), 0);
}

TEST(RenderTicketWatcher, DelegatesGetAndHasResultToTicket)
{
	RenderTicketPtr ticket = std::make_shared<RenderTicket>();
	ticket->Start();
	ticket->Finish(QVariant(QStringLiteral("result")));

	RenderTicketWatcher watcher;
	watcher.SetTicket(ticket);

	EXPECT_FALSE(watcher.IsRunning());
	EXPECT_TRUE(watcher.HasResult());
	EXPECT_EQ(watcher.Get().toString(), QStringLiteral("result"));
}

TEST(RenderTicketWatcher, EmptyWatcherReturnsDefaults)
{
	RenderTicketWatcher watcher;
	EXPECT_FALSE(watcher.IsRunning());
	EXPECT_FALSE(watcher.HasResult());
	EXPECT_TRUE(watcher.Get().isNull());
	EXPECT_EQ(watcher.GetTicket(), nullptr);
}

TEST(RenderTicketWatcher, SettingTicketTwiceIsRejected)
{
	RenderTicketPtr first = std::make_shared<RenderTicket>();
	RenderTicketPtr second = std::make_shared<RenderTicket>();

	RenderTicketWatcher watcher;
	watcher.SetTicket(first);
	watcher.SetTicket(second);

	EXPECT_EQ(watcher.GetTicket(), first);
}
