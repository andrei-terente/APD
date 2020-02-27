import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Class that implements the channel used by headquarters and space explorers to communicate.
 */
public class CommunicationChannel {
	private static final int QUEUE_CAPACITY = 100000;

	private ArrayBlockingQueue<Message> seBuffer;
	private ArrayBlockingQueue<Message> hqBuffer;

	private ReentrantLock hqWriteLock;
	private ReentrantLock hqReadLock;
	private int hqWriteLockCount;


	/**
	 * Creates a {@code CommunicationChannel} object.
	 */
	public CommunicationChannel() {
		seBuffer = new ArrayBlockingQueue<>(QUEUE_CAPACITY, true);
		hqBuffer = new ArrayBlockingQueue<>(QUEUE_CAPACITY, true);

		hqWriteLockCount = 0;
		hqWriteLock = new ReentrantLock();
		hqReadLock = new ReentrantLock();
	}

	/**
	 * Puts a message on the space explorer channel (i.e., where space explorers write to and 
	 * headquarters read from).
	 * 
	 * @param message
	 *            message to be put on the channel
	 */
	public void putMessageSpaceExplorerChannel(Message message) {
		// Using BlockingQueue does the job
		try {
			seBuffer.put(message);
		} catch (InterruptedException e) {
		}
	}

	/**
	 * Gets a message from the space explorer channel (i.e., where space explorers write to and
	 * headquarters read from).
	 * 
	 * @return message from the space explorer channel
	 */
	public Message getMessageSpaceExplorerChannel() {
		// Using BlockingQueue does the job
		Message ret = null;

		try {
			ret = seBuffer.take();
		} catch (InterruptedException e) {
		}

		return ret;
	}

	/**
	 * Puts a message on the headquarters channel (i.e., where headquarters write to and 
	 * space explorers read from).
	 * 
	 * @param message
	 *            message to be put on the channel
	 */
	public void putMessageHeadQuarterChannel(Message message) {
		// Skip END messages, as they are not useful for this
		// implementation
		if (message.getData().equals(HeadQuarter.END)) {
			// Safety measure, i guess
			for (int i = 0; i < hqWriteLock.getHoldCount(); ++i) {
				hqWriteLock.unlock();
			}
			return;
		}

		// Locks the buffer and keeps it locked
		// until the 2 messages are sent
		hqWriteLock.lock();
		try {
			hqBuffer.put(message);
		} catch (InterruptedException e) {
		}

		// holdCount == 2 after a pair of messages was sent
		// the order of the messages is correct so we can unlock
		if (hqWriteLock.getHoldCount() == 2) {
			hqWriteLock.unlock();
			hqWriteLock.unlock();
		}
	}

	/**
	 * Gets a message from the headquarters channel (i.e., where headquarters write to and
	 * space explorer read from).
	 * 
	 * @return message from the header quarter channel
	 */
	public Message getMessageHeadQuarterChannel() {
		Message ret = null;

		// Locks the buffer and keeps it locked
		// until the 2 messages are retrieved
		// by the same explorer
		hqReadLock.lock();
		try {
			ret = hqBuffer.take();
		} catch (InterruptedException e) {
		}

		// holdCount == 2 after a pair of messages was retrieved
		// two messages received by the same explorer, we can unlock
		if (hqReadLock.getHoldCount() == 2) {
			hqReadLock.unlock();
			hqReadLock.unlock();
		}

		return ret;
	}
}
