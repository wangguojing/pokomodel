// LGEOTables.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tinyxml.h>
#include <map>
#include <string>
#include <vector>

struct Element
{
	std::string lgeoName;
	std::string lgeoFilename;
	unsigned int flags;
};

struct Color
{
	std::string lgeoName;
	bool transparent;
};

typedef std::map<std::string, Element> ElementMap;
typedef std::map<int, Color> ColorMap;
typedef std::map<std::string, std::string> StringStringMap;
typedef std::map<std::string, StringStringMap> PatternMap;
typedef std::vector<std::string> StringVector;

bool readString(FILE *tableFile, std::string &string)
{
	for (;;)
	{
		int ch =  fgetc(tableFile);

		if (ch == EOF)
		{
			return false;
		}
		else if (ch == 0)
		{
			return true;
		}
		else
		{
			string += (char)ch;
		}
	}
}

bool readLine(FILE *file, std::string &line)
{
	for (;;)
	{
		int ch =  fgetc(file);

		if (ch == EOF)
		{
			return line.size() > 0;
		}
		else if (ch == '\n')
		{
			size_t spot = line.find(';');

			if (spot < line.size())
			{
				line.resize(spot);
			}
			if (line.size() > 0 && line[line.size() - 1] == '\r')
			{
				line.resize(line.size() - 1);
			}
			if (line.find_first_not_of(" \t") >= line.size())
			{
				line.clear();
			}
			return true;
		}
		else
		{
			line += (char)ch;
		}
	}
}

void scanFields(const std::string &line, StringVector &fields)
{
	std::string temp = line;

	while (temp.size() > 0)
	{
		size_t spot = temp.find_first_of(" \t");

		fields.push_back(temp.substr(0, spot));
		if (spot < temp.size())
		{
			temp = temp.substr(temp.find_first_not_of(" \t", spot));
		}
		else
		{
			temp.clear();
		}
	}
}

bool readColorsFile(const char *filename, ColorMap &colors)
{
	FILE *colorsFile = fopen(filename, "rb");

	try
	{
		if (colorsFile != NULL)
		{
			for (;;)
			{
				std::string line;

				line.reserve(64);
				if (!readLine(colorsFile, line))
				{
					// done
					break;
				}
				if (line.size() > 0)
				{
					StringVector fields;
					Color color;
					unsigned int ldrawNum;

					scanFields(line, fields);
					if (fields.size() < 3)
					{
						throw "Error parsing %s.\n";
					}
					if (sscanf(&fields[0][0], "%d", &ldrawNum) != 1)
					{
						throw "Error parsing %s.\n";
					}
					color.lgeoName = fields[1];
					if (toupper(fields[2][0]) == 'T')
					{
						color.transparent = true;
					}
					else
					{
						color.transparent = false;
					}
					colors[ldrawNum] = color;
				}
			}
			fclose(colorsFile);
			return true;
		}
		else
		{
			throw "Error reading %s.\n";
		}
	}
	catch (const char *error)
	{
		printf(error, filename);
		if (colorsFile != NULL)
		{
			fclose(colorsFile);
		}
		return false;
	}
}

bool readOldColorsFile(const char *filename, ColorMap &colors)
{
	FILE *colorsFile = fopen(filename, "rb");

	try
	{
		if (colorsFile != NULL)
		{
			for (;;)
			{
				unsigned char numBuf[2];
				size_t readSize = fread(numBuf, 1, 2, colorsFile);
				Color color;
				unsigned int ldrawNum;

				if (readSize == 0)
				{
					// done
					break;
				}
				else if (readSize != 2)
				{
					throw "Error parsing %s.\n";
				}
				ldrawNum = ((unsigned int)numBuf[0] << 8) |
					(unsigned int)numBuf[1];
				color.lgeoName.reserve(10);
				if (!readString(colorsFile, color.lgeoName))
				{
					throw "Error parsing %s.\n";
				}
				colors[ldrawNum] = color;
			}
			fclose(colorsFile);
			return true;
		}
		else
		{
			throw "Error reading %s.\n";
		}
	}
	catch (const char *error)
	{
		printf(error, filename);
		if (colorsFile != NULL)
		{
			fclose(colorsFile);
		}
		return false;
	}
}

bool readElementsFile(
	const char *filename,
	ElementMap &table)
{
	FILE *elementsFile = fopen(filename, "rb");

	try
	{
		if (elementsFile != NULL)
		{
			for (;;)
			{
				std::string line;

				line.reserve(64);
				if (!readLine(elementsFile, line))
				{
					// done
					break;
				}
				if (line.size() > 0)
				{
					StringVector fields;
					scanFields(line, fields);

					if (fields.size() < 3)
					{
						throw "Error parsing %s.\n";
					}
					// Only parse the LGEO files for right now.
					if (fields[2].find_first_of("lL") < fields[2].size())
					{
						std::string ldrawFilename;
						Element element;

						element.lgeoName = "lg_";
						element.lgeoName += fields[1];
						element.lgeoFilename = element.lgeoName;
						element.lgeoFilename += ".inc";
						element.flags = 0;
						if (fields[2].find_first_of("sS") < fields[2].size())
						{
							element.flags |= 0x01;	// Slope
						}
						ldrawFilename = fields[0];
						ldrawFilename += ".dat";
						table[ldrawFilename] = element;
					}
				}
			}
			fclose(elementsFile);
			return true;
		}
		else
		{
			throw "Error reading %s.\n";
		}
	}
	catch (const char *error)
	{
		printf(error, filename);
		if (elementsFile != NULL)
		{
			fclose(elementsFile);
		}
		return false;
	}
}

bool readOldElementsFile(
	const char *filename,
	ElementMap &table,
	const PatternMap &patterns)
{
	FILE *tableFile = fopen(filename, "rb");

	try
	{
		if (tableFile != NULL)
		{
			for (;;)
			{
				unsigned char numBuf[4];
				unsigned int ldrawNum;
				int flags;
				char ldrawBase[128];
				std::string ldrawFilename;
				Element element;
				size_t readSize = fread(numBuf, 1, 4, tableFile);
				PatternMap::const_iterator it1;

				if (readSize == 0)
				{
					// done
					break;
				}
				else if (readSize != 4)
				{
					throw "Error parsing %s.\n";
				}
				ldrawNum = ((unsigned int)numBuf[0] << 24) |
					((unsigned int)numBuf[1] << 16) |
					((unsigned int)numBuf[2] << 8) |
					(unsigned int)numBuf[3];
				flags = fgetc(tableFile);
				if (flags == EOF)
				{
					throw "Error parsing %s.\n";
				}
				element.flags = (unsigned int)flags;
				element.lgeoName.reserve(10);
				if (!readString(tableFile, element.lgeoName))
				{
					throw "Error parsing %s.\n";
				}
				element.lgeoFilename = element.lgeoName + ".inc";
				sprintf(ldrawBase, "%d", ldrawNum);
				ldrawFilename = ldrawBase;
				ldrawFilename += ".dat";
				table[ldrawFilename] = element;
				it1 = patterns.find(element.lgeoName.substr(3));
				if (it1 != patterns.end())
				{
					const StringStringMap &strings = it1->second;
					for (StringStringMap::const_iterator it2 = strings.begin();
						it2 != strings.end(); it2++)
					{
						element.lgeoFilename = "patterns/";
						element.lgeoFilename += it2->second;
						element.lgeoFilename += ".inc";
						element.lgeoName = "lg_";
						element.lgeoName += it1->first;
						element.lgeoName += 'p';
						element.lgeoName += it2->first;
						ldrawFilename = ldrawBase;
						ldrawFilename += 'p';
						ldrawFilename += it2->first;
						ldrawFilename += ".dat";
						table[ldrawFilename] = element;
					}
				}
			}
			fclose(tableFile);
			return true;
		}
		else
		{
			throw "Error reading %s.\n";
		}
	}
	catch (const char *error)
	{
		printf(error, filename);
		if (tableFile != NULL)
		{
			fclose(tableFile);
		}
		return false;
	}
}

bool readOldPatternsFile(const char *filename, PatternMap &patterns)
{
	FILE *tableFile = fopen(filename, "rb");

	try
	{
		if (tableFile != NULL)
		{
			for (;;)
			{
				std::string leftName;
				std::string rightName;
				std::string baseName;
				std::string basePattern;
				size_t index;

				leftName.reserve(10);
				if (!readString(tableFile, leftName))
				{
					if (leftName.size() == 0)
					{
						break;
					}
					else
					{
						throw "Error parsing %s.\n";
					}
				}
				rightName.reserve(10);
				if (!readString(tableFile, rightName))
				{
					throw "Error parsing %s.\n";
				}
				index = leftName.find('p');
				if (index >= leftName.size())
				{
					throw "Error parsing %s.\n";
				}
				baseName = leftName.substr(0, index);
				basePattern = leftName.substr(index + 1);
				StringStringMap &map = patterns[baseName];
				map[basePattern] = rightName;
			}
			fclose(tableFile);
			return true;
		}
		else
		{
			throw "Error reading %s.\n";
		}
	}
	catch (const char *error)
	{
		printf(error, filename);
		if (tableFile != NULL)
		{
			fclose(tableFile);
		}
		return false;
	}
}

TiXmlElement *createXmlRootElement(TiXmlDocument &doc)
{
	TiXmlDeclaration *decl = new TiXmlDeclaration( "1.0", "", "" );
	TiXmlElement *rootElement = new TiXmlElement("LDrawPOV");

	doc.LinkEndChild(decl);
	doc.LinkEndChild(rootElement);
	return rootElement;
}

TiXmlElement *addElement(
	TiXmlElement *parent,
	const char *name,
	const char *value = NULL)
{
	TiXmlElement *child = new TiXmlElement(name);

	if (value)
	{
		TiXmlText *text = new TiXmlText(value);
		child->LinkEndChild(text);
	}
	parent->LinkEndChild(child);
	return child;
}

TiXmlElement *addElement(
	TiXmlElement *parent,
	const char *name,
	const std::string &value)
{
	return addElement(parent, name, value.c_str());
}

void addXmlDependencies(TiXmlElement *rootElement, bool old)
{
	TiXmlElement *dependenciesElement = new TiXmlElement("Dependencies");
	TiXmlElement *lgQualityElement = new TiXmlElement("LGQuality");
	TiXmlElement *lgStudsElement = new TiXmlElement("LGStuds");
	TiXmlElement *lgDefsElement = new TiXmlElement("LGDefs");
	TiXmlElement *lgColorsElement = new TiXmlElement("LGColors");

	dependenciesElement->LinkEndChild(lgQualityElement);
	dependenciesElement->LinkEndChild(lgStudsElement);
	dependenciesElement->LinkEndChild(lgDefsElement);
	dependenciesElement->LinkEndChild(lgColorsElement);
	addElement(lgQualityElement, "POVCode", "#declare lg_quality = LDXQual;\n"
		"#if (lg_quality = 3)\n"
		"#declare lg_quality = 4;\n"
		"#end");
	addElement(lgStudsElement, "POVCode", "#declare lg_studs = LDXStuds;\n");
	if (old)
	{
		addElement(lgDefsElement, "POVVersion", "3.0");
	}
	addElement(lgDefsElement, "Dependency", "LGQuality");
	addElement(lgDefsElement, "Dependency", "LGStuds");
	addElement(lgDefsElement, "POVFilename", "lg_defs.inc");
	if (old)
	{
		addElement(lgColorsElement, "POVVersion", "3.0");
	}
	addElement(lgColorsElement, "Dependency", "LGDefs");
	addElement(lgColorsElement, "POVFilename", "lg_color.inc");
	rootElement->LinkEndChild(dependenciesElement);
}

void addXmlColors(
	TiXmlElement *rootElement,
	const ColorMap &colors,
	bool /*old*/)
{
	TiXmlElement *colorsElement = new TiXmlElement("Colors");

	for (ColorMap::const_iterator it = colors.begin(); it != colors.end(); it++)
	{
		unsigned int ldrawNum = it->first;
		const Color &color = it->second;
		TiXmlElement *colorElement = new TiXmlElement("Color");
		char numberBuf[128];

		sprintf(numberBuf, "%d", ldrawNum);
		addElement(colorElement, "LDrawNumber", numberBuf);
		addElement(colorElement, "POVName", color.lgeoName);
		//if (old)
		//{
		//	addElement(colorElement, "POVVersion", "3.0");
		//}
		addElement(colorElement, "Dependency", "LGColors");
		if (color.transparent)
		{
			addElement(colorElement, "IoR", "lg_ior");
		}
		colorsElement->LinkEndChild(colorElement);
	}
	rootElement->LinkEndChild(colorsElement);
}

void addXmlElements(
	TiXmlElement *rootElement,
	const ElementMap &elementMap,
	bool /*old*/)
{
	TiXmlElement *matricesElement = new TiXmlElement("Matrices");
	TiXmlElement *elementsElement = new TiXmlElement("Elements");

	addElement(matricesElement, "LGEOTransform", "0,0,-25,0,-25,0,0,0,0,-25,0,0,0,0,0,1");
	rootElement->LinkEndChild(matricesElement);
	for (ElementMap::const_iterator it = elementMap.begin();
		it != elementMap.end(); it++)
	{
		const std::string &ldrawFilename = it->first;
		const Element &element = it->second;
		TiXmlElement *elementElement = addElement(elementsElement, "Element");
		addElement(elementElement, "LDrawFilename", ldrawFilename);
		addElement(elementElement, "POVName", element.lgeoName);
		TiXmlElement *nameElement = addElement(elementElement, "POVName",
			element.lgeoName + "_clear");
		nameElement->SetAttribute("Alternate", "Clear");
		if (element.flags & 0x01)
		{
			nameElement = addElement(elementElement, "POVName",
				element.lgeoName + "_slope");
			nameElement->SetAttribute("Texture", "Slope");
		}
		addElement(elementElement, "Dependency", "LGDefs");
		addElement(elementElement, "POVFilename", element.lgeoFilename);
		addElement(elementElement, "MatrixRef", "LGEOTransform");
	}
	rootElement->LinkEndChild(elementsElement);
}

void processFiles(const char *path)
{
	PatternMap patterns;
	ElementMap elements;
	ColorMap colors;
	std::string prefix = path;
	std::string colorsFilename;
	FILE *colorsFile;
	bool ready = false;
	bool old = false;

	if (prefix.size() > 0)
	{
		char lastChar = prefix[prefix.size() - 1];

		if (lastChar != '/' && lastChar != '\\')
		{
			prefix += '/';
		}
	}
	colorsFilename = prefix + "lg_colors.lst";
	colorsFile = fopen(colorsFilename.c_str(), "rb");
	if (colorsFile != NULL)
	{
		std::string elementsFilename = prefix + "lg_elements.lst";

		fclose(colorsFile);
		if (readColorsFile(colorsFilename.c_str(), colors) &&
			readElementsFile(elementsFilename.c_str(), elements))
		{
			ready = true;
		}
	}
	else
	{
		old = true;
		colorsFilename = prefix + "l2p_colr.tab";
		colorsFile = fopen(colorsFilename.c_str(), "rb");
		if (colorsFile != NULL)
		{
			std::string elementsFilename = prefix + "l2p_elmt.tab";
			std::string patternsFilename = prefix + "l2p_ptrn.tab";

			fclose(colorsFile);
			if (readOldColorsFile(colorsFilename.c_str(), colors) &&
				readOldPatternsFile(patternsFilename.c_str(), patterns) &&
				readOldElementsFile(elementsFilename.c_str(), elements, patterns))
			{
				ready = true;
			}
		}
	}
	if (ready)
	{
		std::string xmlFilename(prefix + "LGEO.xml");

		TiXmlDocument doc;
		TiXmlElement *rootElement = createXmlRootElement(doc);
		addXmlDependencies(rootElement, old);
		addXmlColors(rootElement, colors, old);
		addXmlElements(rootElement, elements, old);
		doc.SaveFile(xmlFilename);
	}
}

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		processFiles("");
	}
	else if (argc == 2)
	{
		processFiles(argv[1]);
	}
	else
	{
		printf("Usage: LGEOTables [LGEO Path]\n");
		return 1;
	}
	return 0;
}

