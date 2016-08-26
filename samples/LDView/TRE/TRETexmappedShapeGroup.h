#ifndef __TRETEXMAPPEDSHAPEGROUP_H__
#define __TRETEXMAPPEDSHAPEGROUP_H__

#include <TRE/TREColoredShapeGroup.h>

// A shape group for handling sorted transparent shapes.
class TRETexmappedShapeGroup: public TREColoredShapeGroup
{
public:
	TRETexmappedShapeGroup(void);
	TRETexmappedShapeGroup(const TRETexmappedShapeGroup &other);
	virtual void draw(void);
	void setStepCounts(const IntVector &value);
	void stepChanged(void);
protected:
	~TRETexmappedShapeGroup(void);
	virtual void dealloc(void);

	TCULongArray *m_origIndices;
};

#endif // __TRETEXMAPPEDSHAPEGROUP_H__

