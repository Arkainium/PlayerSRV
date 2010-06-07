#ifndef METROBOTICS_SURVEYOR_IRARRAY_H
#define METROBOTICS_SURVEYOR_IRARRAY_H

class IRArray
{
	public:
		IRArray(int front = 0, int left = 0, int back = 0, int right = 0)
			:mFront(front), mLeft(left), mBack(back), mRight(right)
			{ }
		int front() const { return mFront; }
		int left()  const { return mLeft; }
		int back()  const { return mBack; }
		int right() const { return mRight; }

	private:
		int mFront;
		int mLeft;
		int mBack;
		int mRight;
};

#endif
