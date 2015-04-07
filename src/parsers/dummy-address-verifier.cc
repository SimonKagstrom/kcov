#include "address-verifier.hh"

using namespace kcov;

class AddressVerifier : public IAddressVerifier
{
public:
	virtual void setup(const void *header, size_t headerSize)
	{
	}

	bool verify(const void *sectionData, size_t sectionSize, uint64_t offset)
	{
#if defined(__arm__)
		return (offset & 1) == 0; // Thumb-conservative
#elif defined(__powerpc__)
		return (offset & 3) == 0;
#endif

		return true;
	}
};

IAddressVerifier *IAddressVerifier::create()
{
	return new AddressVerifier();
}
