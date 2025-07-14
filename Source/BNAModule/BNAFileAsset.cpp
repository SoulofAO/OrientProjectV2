#include "BNAFileAsset.h"
#include "Misc/FileHelper.h"
#include "XmlFile.h"
#include "OrientProject/UpgradeBlueprintFunctionLibrary.h"

UClass* FBNADataAssetTypeActions::GetSupportedClass() const
{
	return UBNADataObject::StaticClass();
}

FText FBNADataAssetTypeActions::GetName() const
{
	return FText::FromString(FString("BNADataObject"));
}

FColor FBNADataAssetTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

uint32 FBNADataAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

UBNADataAssetFactory::UBNADataAssetFactory()
{
	SupportedClass = UBNADataObject::StaticClass();
	bCreateNew = false;
    bEditorImport = true;
    bText = true;
    Formats.Add(TEXT("bna;Just BNA"));
}

UObject* UBNADataAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UBNADataObject>(InParent, Class, Name, Flags, Context);
}

bool UBNADataAssetFactory::FactoryCanImport(const FString& Filename)
{
	return FPaths::GetExtension(Filename) == TEXT("bna");
}

bool UBNADataAssetFactory::CanConvertToFloat(const FString& Str)
{
    return Str.IsNumeric();
}

UObject* UBNADataAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UBNADataObject* LNewObject = NewObject<UBNADataObject>(InParent, InClass, InName, Flags);
    if (IsValid(LNewObject))
    {
        FString FileContent;
        TMap<FName, FLineStruct> NewDataMap;

        if (FFileHelper::LoadFileToString(FileContent, *Filename))
        {
            UE_LOG(LogTemp, Log, TEXT("Содержимое файла: %s"), *FileContent);

            FPointArrayStruct PointArray;

            FName Name = "";
            TArray<FString> LineArray;
            int IndividualIndex = -1;
            FileContent.ParseIntoArrayLines(LineArray);

            int Counter = 0;
            for (const FString& Line : LineArray)
            {
                FString StrippedLine = Line.TrimStartAndEnd();
                TArray<FString> Points;

                StrippedLine.ParseIntoArray(Points, TEXT(","), true);
                FString ClassName = "";
                if (Points.Num() > 2)
                {
                    TArray<FString> SelectedPoints;
                    for (int32 i = 0; i < Points.Num() - 2; ++i)
                    {
                        SelectedPoints.Add(Points[i]);
                    }
                    ClassName = FString::Join(SelectedPoints, TEXT(","));
                }
                else if (Points.Num() <= 2)
                {
                    ClassName = Points[0];
                }
                if (GetMutableDefault<UOrientiringImportAssetDeveloperSettings>()->IgnoringNames.Contains(Name))
                {
                    continue;
                }

                if (!CanConvertToFloat(ClassName))
                {
                    if (PointArray.Points.Num() > 0)
                    {
                        if (NewDataMap.Contains(Name))
                        {
                            FLineStruct* LineStruct = NewDataMap.Find(Name);
                            LineStruct->Line.Add(IndividualIndex, PointArray);
                        }
                        else
                        {
                            FLineStruct NewLineStruct;
                            NewLineStruct.Line.Add(IndividualIndex, PointArray);
                            NewDataMap.Add(Name, NewLineStruct);
                        }
                    }
                    Name = FName(ClassName);
                    PointArray = FPointArrayStruct();
                    IndividualIndex = Counter;
                }
                else
                {
                    if (Points.Num() == 2)
                    {
                        float X = FCString::Atof(*Points[0]);
                        float Y = FCString::Atof(*Points[1]);
                        PointArray.Points.Add(FVector2D(X, Y));
                    }
                }

                Counter++;
            }
            if (PointArray.Points.Num() > 0)
            {
                if (NewDataMap.Contains(Name))
                {
                    FLineStruct* LineStruct = NewDataMap.Find(Name);
                    LineStruct->Line.Add(IndividualIndex, PointArray);
                }
                else
                {
                    FLineStruct NewLineStruct;
                    NewLineStruct.Line.Add(IndividualIndex, PointArray);
                    NewDataMap.Add(Name, NewLineStruct);
                }
            }
        }
        LNewObject->DataMap = NewDataMap;
    }
	return LNewObject;
}

UOMAPDataAssetFactory::UOMAPDataAssetFactory()
{
    SupportedClass = UBNADataObject::StaticClass();
    bCreateNew = false;
    bEditorImport = true;
    bText = true;
    Formats.Add(TEXT("omap;Just omap"));
}

UObject* UOMAPDataAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UBNADataObject>(InParent, Class, Name, Flags, Context);
}

bool UOMAPDataAssetFactory::FactoryCanImport(const FString& Filename)
{
    return FPaths::GetExtension(Filename) == TEXT("omap");
}

UObject* UOMAPDataAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    UBNADataObject* LNewObject = NewObject<UBNADataObject>(InParent, InClass, InName, Flags);
    if (IsValid(LNewObject))
    {
        LNewObject->ImportSource = FPaths::ConvertRelativePathToFull(Filename);;

        FString FilePath = FPaths::ConvertRelativePathToFull(Filename);
        FXmlFile XmlFile(FilePath, EConstructMethod::ConstructFromFile);
        if (XmlFile.IsValid())
        {
            FXmlNode* RootNode = XmlFile.GetRootNode();
            if (RootNode)
            {
                int Counter = 0;

                TMap<int, FString> NameBySymbolIDMap;
                FXmlNode* Georeferencing = FindXmlNodeByName(RootNode, "georeferencing");
                if (Georeferencing)
                {
                    float Scale = FCString::Atof(*Georeferencing->GetAttribute("scale"));
                    LNewObject->Scale = Scale;
                }

                FXmlNode* SymbolNode = FindXmlNodeByName(RootNode, "symbols");
                for (FXmlNode* ObjectXmlNode : SymbolNode->GetChildrenNodes())
                {
                    int SymbolId = FCString::Atoi(*ObjectXmlNode->GetAttribute("id"));
                    FString Name = ObjectXmlNode->GetAttribute("name");
                    NameBySymbolIDMap.Add(TPair<int, FString>(SymbolId, Name));
                }

                FXmlNode* ObjectsNode = FindXmlNodeByName(RootNode, "objects");
                for (FXmlNode* ObjectXmlNode : ObjectsNode->GetChildrenNodes())
                {
                    int SymbolId = -1;
                    TArray<FString> CoordPairs;

                    SymbolId = FCString::Atoi(*ObjectXmlNode->GetAttribute("symbol"));
                    if (!NameBySymbolIDMap.Contains(SymbolId))
                    {
                        continue;
                    }
                    FString Name = *NameBySymbolIDMap.Find(SymbolId);
                    if (GetMutableDefault<UOrientiringImportAssetDeveloperSettings>()->IgnoringNames.Contains(Name))
                    {
                        continue;
                    }

                    float Rotation = FCString::Atof(*ObjectXmlNode->GetAttribute("rotation"));

                    FString Text;

                    if (ObjectXmlNode->GetChildrenNodes().IsValidIndex(1))
                    {
                        Text = ObjectXmlNode->GetChildrenNodes()[1]->GetContent();
                    }

                    const FXmlNode* FirstCoord = ObjectXmlNode->GetFirstChildNode();
                    FString CoordsString = FirstCoord->GetContent();

                    CoordsString.ParseIntoArray(CoordPairs, TEXT(";"), true);
                    
                    /*
                    if (CoordPairs.Num() > 1 && CoordPairs[CoordPairs.Num() - 1] == CoordPairs[0])
                    {
                        CoordPairs.RemoveAt(CoordPairs.Num() - 1);
                    }
                    */

                    TArray<FVector2D> ParsedCoords;
                    TArray<TArray<FVector2D>> PopLines;

                    ELineStatus LineStatus = ELineStatus::Base;

                    int PairIndex = 0;
                    for (const FString& Pair : CoordPairs)
                    {
                        TArray<FString> SplitCoords;
                        Pair.ParseIntoArray(SplitCoords, TEXT(" "), true);

                        if (SplitCoords.Num()>=2)
                        {
                            float X = FCString::Atof(*SplitCoords[0]) / 100.0f;
                            float Y = FCString::Atof(*SplitCoords[1]) / 100.0f*-1.0;

                            if (LNewObject->Scale != 0)
                            {
                                X = X * LNewObject->Scale / 10000 * 100;
                                Y = Y * LNewObject->Scale / 10000 * 100;
                            }

                            if (LineStatus == ELineStatus::Pop)
                            {
                                PopLines[PopLines.Num() - 1].Add(FVector2D(X, Y));
                            }
                            else
                            {
                                ParsedCoords.Add(FVector2D(X, Y));
                            }

                            if (SplitCoords.Num() == 3)
                            {
                                float InstrumentIndex = FCString::Atof(*SplitCoords[2]);

                                if (InstrumentIndex == 18.0)
                                {
                                    LineStatus = ELineStatus::Pop;
                                    PopLines.Add(TArray<FVector2D>());
                                }
                            }
                        }
                        PairIndex = PairIndex + 1;
                    }

                    auto FindClosestPair = [](const TArray<FVector2D>& A, const TArray<FVector2D>& B) -> TPair<FVector2D, FVector2D>
                        {
                            float MinDistSqr = TNumericLimits<float>::Max();
                            FVector2D ClosestA, ClosestB;

                            for (const FVector2D& PointA : A)
                            {
                                for (const FVector2D& PointB : B)
                                {
                                    float DistSqr = FVector2D::DistSquared(PointA, PointB);
                                    if (DistSqr < MinDistSqr)
                                    {
                                        MinDistSqr = DistSqr;
                                        ClosestA = PointA;
                                        ClosestB = PointB;
                                    }
                                }
                            }

                            return TPair<FVector2D, FVector2D>(ClosestA, ClosestB);
                        };


                    TMap<TPair<FVector2D, FVector2D>, TArray<FVector2D>*> NearestPairPointMap;

                    for (TArray<FVector2D>& PopLine : PopLines)
                    {
                        if (PopLine.Num() <= 1)
                        {
                            continue;
                        }

                        TPair<FVector2D, FVector2D> ClosestPoint = FindClosestPair(ParsedCoords, PopLine);
                        NearestPairPointMap.Add(ClosestPoint, &PopLine);
                    }

                    for (TPair<TPair<FVector2D, FVector2D>, TArray<FVector2D>*> NearestPairPoint : NearestPairPointMap)
                    {
                        if (!ParsedCoords.Contains(NearestPairPoint.Key.Key))
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Error - Find Point A"));
                        }
                        if (!NearestPairPoint.Value->Contains(NearestPairPoint.Key.Value))
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Error - Find Point B"));
                        }

                        int IndexIn = ParsedCoords.Find(NearestPairPoint.Key.Key); 
                        FVector2D StartPoint = ParsedCoords[FMath::Max(0,IndexIn - 1)];
                        int IndexOut = NearestPairPoint.Value->Find(NearestPairPoint.Key.Value);

                        for (int Index = IndexOut; Index < (NearestPairPoint.Value->Num() + IndexOut); Index++)
                        {
                            int RealIndex = Index % (NearestPairPoint.Value->Num() - 1);
                            ParsedCoords.Insert((*NearestPairPoint.Value)[RealIndex], IndexIn);
                            IndexIn++;
                        }
                        ParsedCoords.Insert(StartPoint, IndexIn);
                    }

                    if (NearestPairPointMap.Num() > 0 && ParsedCoords[0] != ParsedCoords[ParsedCoords.Num() - 1])
                    {
                        const FVector2D ClosedPosition = ParsedCoords[0];
                        ParsedCoords.Add(ClosedPosition);
                    }

                    if (!LNewObject->DataMap.Contains(FName(*Name)))
                    {
                        TMap<int, FPointArrayStruct> Map;
                        LNewObject->DataMap.Add(TPair<FName, FLineStruct>(Name, Map));
                    }
                    FPointArrayStruct PointArrayStruct;
                    FLineStruct* LineStruct = LNewObject->DataMap.Find(FName(*Name));
                    FPointArrayStruct NewPointArrayStruct;
                    NewPointArrayStruct.Points = ParsedCoords;
                    NewPointArrayStruct.Rotation = Rotation;
                    NewPointArrayStruct.Text = Text;
                    LineStruct->Line.Add(TPair<int, FPointArrayStruct>(Counter, NewPointArrayStruct));

                    Counter++;
                }
            }
        }
    }
    return LNewObject;
}

FXmlNode* UOMAPDataAssetFactory::FindXmlNodeByName(FXmlNode* CurrentNode, const FString& TargetName)
{
    if (CurrentNode->GetTag() == TargetName)
    {
        return CurrentNode;
    }

    TArray<FXmlNode*> ChildrenNodes = CurrentNode->GetChildrenNodes();
    for (FXmlNode* ChildNode : ChildrenNodes)
    {
        FXmlNode* FoundNode = FindXmlNodeByName(ChildNode, TargetName);
        if (FoundNode != nullptr)
        {
            return FoundNode;
        }
    }

    return nullptr;
}

FName UOrientiringImportAssetDeveloperSettings::GetCategoryName() const
{
    return FName("Orientiring");
}

FName UOrientiringImportAssetDeveloperSettings::GetSectionName() const
{
    return FName("OrientiringSettings");
}

FVector2D UBNADataObject::GetMinPosition()
{
    TArray<FVector2D> Points;
    for (TPair< FName, FLineStruct> Pair : DataMap)
    {
        for (TPair<int, FPointArrayStruct> PointPair : Pair.Value.Line)
        {
            for (FVector2D Point : PointPair.Value.Points)
            {
                Points.Add(Point);
            }
        }
    }
    return UUpgradeBlueprintFunctionLibrary::GetMinVector(Points);
}

FVector2D UBNADataObject::GetMaxPosition()
{
    TArray<FVector2D> Points;
    for (TPair< FName, FLineStruct> Pair : DataMap)
    {
        for (TPair<int, FPointArrayStruct> PointPair : Pair.Value.Line)
        {
            for (FVector2D Point : PointPair.Value.Points)
            {
                Points.Add(Point);
            }
        }
    }
    return UUpgradeBlueprintFunctionLibrary::GetMaxVector(Points);
}
