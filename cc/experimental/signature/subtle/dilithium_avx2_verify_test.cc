// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////

#include "tink/experimental/signature/subtle/dilithium_avx2_verify.h"

#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "tink/config/tink_fips.h"
#include "tink/experimental/signature/subtle/dilithium_avx2_sign.h"
#include "tink/public_key_sign.h"
#include "tink/public_key_verify.h"
#include "tink/util/secret_data.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"
#include "tink/util/test_matchers.h"

extern "C" {
#include "third_party/pqclean/crypto_sign/dilithium2/avx2/sign.h"
}

namespace crypto {
namespace tink {
namespace subtle {
namespace {

using ::crypto::tink::test::IsOk;
using ::crypto::tink::test::StatusIs;
using crypto::tink::util::Status;

class DilithiumAvx2VerifyTest : public ::testing::Test {};

TEST_F(DilithiumAvx2VerifyTest, InvalidPublicKeys) {
  if (IsFipsModeEnabled()) {
    GTEST_SKIP() << "Test assumes kOnlyUseFips is false.";
  }

  // Null public key.
  const absl::string_view null_public_key;
  EXPECT_FALSE(DilithiumAvx2Verify::New(
                   *DilithiumPublicKey::NewPublicKey(null_public_key))
                   .ok());

  for (int keysize = 0; keysize < 1312; keysize++) {
    if (keysize == PQCLEAN_DILITHIUM2_AVX2_CRYPTO_SECRETKEYBYTES) {
      // Valid key size.
      continue;
    }
    std::string key(keysize, 'x');
    EXPECT_FALSE(
        DilithiumAvx2Verify::New(*DilithiumPublicKey::NewPublicKey(key)).ok());
  }
}

TEST_F(DilithiumAvx2VerifyTest, BasicSignVerify) {
  if (IsFipsModeEnabled()) {
    GTEST_SKIP() << "Test assumes kOnlyUseFips is false.";
  }

  // Generate key pair.
  util::StatusOr<std::pair<DilithiumPrivateKey, DilithiumPublicKey>> key_pair =
      DilithiumPrivateKey::GenerateKeyPair();

  ASSERT_THAT(key_pair.status(), IsOk());

  // Create a new signer.
  util::StatusOr<std::unique_ptr<PublicKeySign>> signer =
      DilithiumAvx2Sign::New((*key_pair).first);
  ASSERT_THAT(signer.status(), IsOk());

  // Create a new verifier.
  absl::StatusOr<std::unique_ptr<PublicKeyVerify>> verifier =
      DilithiumAvx2Verify::New((*key_pair).second);
  ASSERT_THAT(verifier.status(), IsOk());

  // Sign a message.
  std::string message = "message to be signed";
  absl::StatusOr<std::string> signature = (*std::move(signer))->Sign(message);
  ASSERT_THAT(signature.status(), IsOk());

  // Verify signature.
  Status status = (*verifier)->Verify(*signature, message);
  EXPECT_THAT(status, IsOk());
}

TEST_F(DilithiumAvx2VerifyTest, FailsWithWrongMessage) {
  if (IsFipsModeEnabled()) {
    GTEST_SKIP() << "Test assumes kOnlyUseFips is false.";
  }

  // Generate key pair.
  util::StatusOr<std::pair<DilithiumPrivateKey, DilithiumPublicKey>> key_pair =
      DilithiumPrivateKey::GenerateKeyPair();

  ASSERT_THAT(key_pair.status(), IsOk());

  // Create a new signer.
  util::StatusOr<std::unique_ptr<PublicKeySign>> signer =
      DilithiumAvx2Sign::New((*key_pair).first);
  ASSERT_THAT(signer.status(), IsOk());

  // Create a new verifier.
  absl::StatusOr<std::unique_ptr<PublicKeyVerify>> verifier =
      DilithiumAvx2Verify::New((*key_pair).second);
  ASSERT_THAT(verifier.status(), IsOk());

  // Sign a message.
  std::string message = "message to be signed";
  absl::StatusOr<std::string> signature = (*std::move(signer))->Sign(message);
  ASSERT_THAT(signature.status(), IsOk());

  // Verify signature.
  Status status = (*verifier)->Verify(*signature, "some bad message");
  EXPECT_FALSE(status.ok());
}

TEST_F(DilithiumAvx2VerifyTest, FailsWithWrongSignature) {
  if (IsFipsModeEnabled()) {
    GTEST_SKIP() << "Test assumes kOnlyUseFips is false.";
  }

  // Generate key pair.
  util::StatusOr<std::pair<DilithiumPrivateKey, DilithiumPublicKey>> key_pair =
      DilithiumPrivateKey::GenerateKeyPair();

  ASSERT_THAT(key_pair.status(), IsOk());

  // Create a new signer.
  util::StatusOr<std::unique_ptr<PublicKeySign>> signer =
      DilithiumAvx2Sign::New((*key_pair).first);
  ASSERT_THAT(signer.status(), IsOk());

  // Create a new verifier.
  absl::StatusOr<std::unique_ptr<PublicKeyVerify>> verifier =
      DilithiumAvx2Verify::New((*key_pair).second);
  ASSERT_THAT(verifier.status(), IsOk());

  // Sign a message.
  std::string message = "message to be signed";
  absl::StatusOr<std::string> signature = (*std::move(signer))->Sign(message);
  ASSERT_THAT(signature.status(), IsOk());

  // Verify signature.
  Status status =
      (*verifier)->Verify(*signature + "some trailing data", message);
  EXPECT_FALSE(status.ok());
}

TEST_F(DilithiumAvx2VerifyTest, FailsWithByteFlipped) {
  if (IsFipsModeEnabled()) {
    GTEST_SKIP() << "Test assumes kOnlyUseFips is false.";
  }

  // Generate key pair.
  util::StatusOr<std::pair<DilithiumPrivateKey, DilithiumPublicKey>> key_pair =
      DilithiumPrivateKey::GenerateKeyPair();

  ASSERT_THAT(key_pair.status(), IsOk());

  // Create a new signer.
  util::StatusOr<std::unique_ptr<PublicKeySign>> signer =
      DilithiumAvx2Sign::New((*key_pair).first);
  ASSERT_THAT(signer.status(), IsOk());

  // Create a new verifier.
  absl::StatusOr<std::unique_ptr<PublicKeyVerify>> verifier =
      DilithiumAvx2Verify::New((*key_pair).second);
  ASSERT_THAT(verifier.status(), IsOk());

  // Sign a message.
  std::string message = "message to be signed";
  absl::StatusOr<std::string> signature = (*std::move(signer))->Sign(message);
  ASSERT_THAT(signature.status(), IsOk());

  // Invalidate one signature byte.
  (*signature)[0] ^= 1;

  // Verify signature.
  Status status = (*verifier)->Verify("some bad signature", message);
  EXPECT_FALSE(status.ok());
}

TEST_F(DilithiumAvx2VerifyTest, FipsMode) {
  if (!IsFipsModeEnabled()) {
    GTEST_SKIP() << "Test assumes kOnlyUseFips.";
  }

  // Generate key pair.
  util::StatusOr<std::pair<DilithiumPrivateKey, DilithiumPublicKey>> key_pair =
      DilithiumPrivateKey::GenerateKeyPair();

  ASSERT_THAT(key_pair.status(), IsOk());

  EXPECT_THAT(DilithiumAvx2Verify::New((*key_pair).second).status(),
              StatusIs(util::error::INTERNAL));
}

struct TestVector {
  std::string public_key;
  std::string message;
  std::string signature;
};

// Test vectors generated with https://github.com/pq-crystals/dilithium
// commit 9dddb2a0537734e749ec2c8d4f952cb90cd9e67b
TEST_F(DilithiumAvx2VerifyTest, Vectors) {
  TestVector dilithium2_vectors[] = {
      {
          /*TEST 1*/
          /*public_key= */ absl::HexStringToBytes(
              "1C0EE1111B08003F28E65E8B3BDEB037CF8F221DFCDAF5950EDB38D506D85BEF"
              "6177E3DE0D4F1EF5847735947B56D08E841DB2444FA2B729ADEB1417CA7ADF42"
              "A1490C5A097F002760C1FC419BE8325AAD0197C52CED80D3DF18E7774265B289"
              "912CECA1BE3A90D8A4FDE65C84C610864E47DEECAE3EEA4430B9909559408D11"
              "A6ABDB7DB9336DF7F96EAB4864A6579791265FA56C348CB7D2DDC90E133A95C3"
              "F6B13601429F5408BD999AA479C1018159550EC55A113C493BE648F4E036DD4F"
              "8C809E036B4FBB918C2C484AD8E1747AE05585AB433FDF461AF03C25A7737007"
              "21AA05F7379FE7F5ED96175D4021076E7F52B60308EFF5D42BA6E093B3D0815E"
              "B3496646E49230A9B35C8D41900C2BB8D3B446A23127F7E096D85A1C794AD4C8"
              "9277904FC6BFEC57B1CDD80DF9955030FDCA741AFBDAC827B13CCD5403588AF4"
              "644003C2265DFA4D419DBCCD2064892386518BE9D51C16498275EBECF5CDC7A8"
              "20F2C29314AC4A6F08B2252AD3CFB199AA42FE0B4FB571975C1020D949E194EE"
              "1EAD937BFB550BB3BA8E357A029C29F077554602E1CA2F2289CB9169941C3AAF"
              "DB8E58C7F2AC77291FB4147C65F6B031D3EBA42F2ACFD9448A5BC22B476E07CC"
              "CEDA2306C554EC9B7AB655F1D7318C2B7E67D5F69BEDF56000FDA98986B5AB1B"
              "3A22D8DFD6681697B23A55C96E8710F3F98C044FB15F606313EE56C0F1F5CA0F"
              "512E08484FCB358E6E528FFA89F8A866CCFF3C0C5813147EC59AF0470C4AAD01"
              "41D34F101DA2E5E1BD52D0D4C9B13B3E3D87D1586105796754E7978CA1C68A7D"
              "85DF112B7AB921B359A9F03CBD27A7EAC87A9A80B0B26B4C9657ED85AD7FA261"
              "6AB345EB8226F69FC0F48183FF574BCD767B5676413ADB12EA2150A0E97683EE"
              "54243C25B7EA8A718606F86993D8D0DACE834ED341EEB724FE3D5FF0BC8B8A7B"
              "8104BA269D34133A4CF8300A2D688496B59B6FCBC61AE96062EA1D8E5B410C56"
              "71F424417ED693329CD983001FFCD10023D598859FB7AD5FD263547117100690"
              "C6CE7438956E6CC57F1B5DE53BB0DC72CE9B6DEAA85789599A70F0051F1A0E25"
              "E86D888B00DF36BDBC93EF7217C45ACE11C0790D70E9953E5B417BA2FD9A4CAF"
              "82F1FCE6F45F53E215B8355EF61D891DF1C794231C162DD24164B534A9D48467"
              "CDC323624C2F95D4402FF9D66AB1191A8124144AFA35D4E31DC86CAA797C31F6"
              "8B85854CD959C4FAC5EC53B3B56D374B888A9E979A6576B6345EC8522C960699"
              "0281BF3EF7C5945D10FD21A2A1D2E5404C5CF21220641391B98BCF825398305B"
              "56E58B611FE5253203E3DF0D22466A73B3F0FBE43B9A62928091898B8A0E5B26"
              "9DB586B0E4DDEF50D682A12D2C1BE824149AA254C6381BB412D77C3F9AA902B6"
              "88C81715A59C839558556D35ED4FC83B4AB18181F40F73DCD76860D8D8BF9452"
              "0237C2AC0E463BA09E3C9782380DC07FE4FCBA340CC2003439FD231461063807"
              "0D6C9EEA0A70BAE83B5D5D3C5D3FDE26DD01606C8C520158E7E5104020F248CE"
              "AA666457C10AEBF068F8A3BD5CE7B52C6AF0ABD5944AF1AD4752C9113976083C"
              "03B6C34E1D47ED69644CAD782C2F7D05F8A148961D965FA2E1723A8DDEBC22A9"
              "0CD783DD1F4DB38FB9AE5A6714B3D946781643D317B7DD79381CF789A9588BB3"
              "E193B92A0B60D6B07D047F6984B0609EC57543C394CA8D5E5BCC2A731A79618B"
              "D1E2E0DA8704AF98F20F5F8F5452DDF646B95B341DD7F0D2CC1FA15BD9895CD5"
              "B65AA1CB94B5E2E788FDA9825B656639193D98328154A4F2C35495A38B6EA0D2"
              "FFAAA35DF92C203C7F31CBBCA7BD03C3C2302190CECD161FD49237E4F839E3F"
              "3"),

          /*message = */
          absl::HexStringToBytes("D81C4D8D734FCBFBEADE3D3F8A039FAA2A2C9957E835A"
                                 "D55B22E75BF57BB556AC8"),
          /*signature = */
          absl::HexStringToBytes(
              "AF5920774603D20E98A79AA3ABFA32B6E22519E673E37AC4AC73FE85341E2C29"
              "23C1992E1B0BBE3873D7C8FC5662F207BF58EA381CD4A3A0C062DEC45BDAF8BA"
              "0AA52BEF6FA14F3F6CF28F7620BF94A92CC27D045414A64D65C0149630528024"
              "28BF3987A2D47516CA5C78AAB96B7BE11BCA5F2C5A26F3FCE3A26E8E09A2738F"
              "386F75D448F937EF19A846BD4DD949CAAF36DB5629884AF53A023E3F180FE4C0"
              "FAFF7BE5DFE4E89ADE3095A65600421461AD08C129D6CEA851BB39C0D7A7D151"
              "405689A091FA4DEBAC373CF54AE078F0AF7557BBC6F06A535AE8949E0C65308A"
              "59840072375295802D0E2CE9A3DA98426A00FF03FE80218C0EEC8EFE581CB9CC"
              "9A7D66B20645A8CD0490D3CE4F7E6FEAE9C9EB7A57F964D0EBC7C90B7A9F8630"
              "0B3E8095E64D1294CFC4B4D9E272E8FA8DB5707D7004AF22DBFF9CFD4863DF57"
              "3FE004341DA3CD4A3082532C2620455FA37C562BAFD5684EA128AFC79E01FC9B"
              "31E8433BAD7C029F2F13CC10592D2332E3E08B80D350463DE72750B1F806F493"
              "E143BD5FCA7D1698081B31BF876B2A1BC9DF50952D13B6C1321B1111172145A6"
              "27AE0B4427B98975CBFFF7D68275754B45B682D709E168522E84FEA7DD3BB0F4"
              "1505FF71926431D1A90D4CBF9A527AD4E284976FFF8BD9D6224A4F260391A987"
              "FB6DA6EE42C2A4900F407CE1F02E322475D313FBEBB68C2E05730809448A7428"
              "A5940139EBDF1B5556FCC5D42E1A13F32230CB6F0724831D0D071BBA5A670480"
              "6F475B74BA91B6E385D48620958D0AB1BF2B184E10F3E753B71337BE9EB65378"
              "6785B43AC7E5C494AC1BCB043D461425B36098AC93055A0105AB8523B61D024A"
              "6E9B56A42D3C04726512AE4CFE05710446B06F694234EE4FA8FEEDDDC5F28A65"
              "EDE2EB58E965FE3627A571BC45B397ED092AB4BE00041729C4D192FE30678279"
              "D223A848CF4366E92B3F68DEE97C9B4A7FF22F937BE6C56639961DB29FA3CFEC"
              "FFF293140886FFB92EBC79DAB59CEAF869C64F8EAF585CE97DD6B78F892772DB"
              "88A958CF0AB557A7FAA83FE621477E2B84497AB5A8ECF4A7BD32DFB902F05D2C"
              "A31047D0F1919ADDE1EE6DFD58E59BC4DAB3CCBBA36AAAF6AFCCC7B095CA94A1"
              "95BE9A289526B588C3A9C56876FC415D521D442BAC0298D302419AD527DA249C"
              "2A660CD064213FFAD563183F37972578EEB9F70AC67AEE6CC2B71F283A95930B"
              "554738555791C25E7A399E685636D58D69CB6BE793B45C1969E7D5615627EBC3"
              "2EED45440F87880D2829FA4FC871866164D259ED95D2731871017FF51894066F"
              "AE1FFA6F4B4A6F84FCFFDA09E718FA17135EDB3F48558D5BA67F9E6F0900340B"
              "D04DFE59B7BD67745884FB84AE3F8EE763D202743652D4F7333450580490B9C7"
              "44935B19C1D5FB0DB5FBB461411362838037EB7EC3F63F26C893E7CC1C3B3F47"
              "67ABAE00FEB7BB99B1420BB29EA614747896D9EDCF8107FE504C9C308A8264DA"
              "CE318D87CFE4761803E9A60DEFA6144AABC1F10A45B140DED754E73586C467BB"
              "7BF19EDEF25BE0C65E93C5E5EB8F880CCE4A858757F8FF56062B1067F4106F76"
              "B7007F6EA6F945047E85BD0FAD9D26994F678A0612B87CCF9C0CF9A433D889C9"
              "6E4C12BE372277005B06AD127105D16D8FB142AEAE5373ABD61D9ADCFC5550D6"
              "23CA3B8824B0E2E08C2BF4E2841EAC4C5DC56CF8954CF207C263F27C9F309F10"
              "307C0D84A65878425031375DD810D2D7E51098A3814350795C4A077FA40DD44F"
              "0FA7510F7C3F631407CF34F604C7B335632A20D2AD419BD7CC6D4242B1C66C35"
              "E5A5EDCCB13CA37D3B50465F3B4AAFF7E3161E7936088AE08401FD2C37D67A2F"
              "F91D3E6F08686D64BC2FC6C57106E49FA384AC22219F07EE8996CA3DFF59DCC5"
              "092A4BADBE87AEDE7F69A04C79B33BDF35D4A0E4CB4B55019CB0BF275295B93B"
              "DABEA516CA2B616A56918600B724BE7A01EC4EF54312B30D66F507815F2780FF"
              "EE7C30F8425A92252CE550FAB4E902E7B382D46DBD20EFE1BB0EF8A496873C09"
              "C4CEB0303C7F1DABA0102DE94190B6AC6DC810F72BCA3AA292FF38BD51A7FAB8"
              "509EC4FBE0EAA3C986166A674B7871155C348C477EF8CEDC832B5ABEE71A8D18"
              "D06DD0F5221160ABEB71E6E82CFABF731EA3515A76EF07B2C16C63B37F7AB73B"
              "67F005929A753E453B930C0AF432277FD77D8A1EB8022CDE9665763B014F0A67"
              "2A04160B0A06F5540F4C264B7F22740690A2352DC863B588303AD51F0AE162BF"
              "79797F07B534501CBBFDB713A724AA98E19532187180CCFADC6EBE3142FA7DB6"
              "6CD4DE7B9FBD4C8235686DB68CAF489AFA4E1E87AEF0CEFD8037E3A578EE62EB"
              "7F94ED5BC0B58EEA4B4C45FC56D31D29944D095AC96C29083DA2C77181D97A55"
              "FE6E903A2F2783DE0BAA5F47D704785C33E8D5C87ED61E65459167310EB7A995"
              "74EF819AE9161A3BD09634803D9E1E4EC7386D7946984517213AB9CF66AEA551"
              "CC457C39F86AF294CF7B073F563ED4DAB9419BDF004BD05C92B4E80EC3CFEAC9"
              "7E1DDA554FDA625C4B9B039BAA7C5A2F6F97057792483CF5F852D4C3AC71AD50"
              "F779953DCFE2F63ED235D8E1D5345D6C6DF0555CC2631DEAD9B714BC4C16501E"
              "01261381F3679715345123388C852D57DCF1941D0911D49FEA7143FD2FC343A5"
              "075B64CCA48291DC28B83F76074589EAB217C7847840652C0E3AE278B3B6FB0D"
              "800C5E7DB79D5CB9CC1A87450C00B7677812D22EE20FDE8C1753A7FB93BA8BBB"
              "8595A6393DF54AA9CDB6E0879A26E49BD3B01513C6053A0746C8596CE5E5B225"
              "CFCA26AB8BF12F1FE0A647A9E4453039A1226194C46E8B98ACD710F18FB7EC05"
              "476C1CD8FC3112CCDDB1582B8817C18FE315353E7A47C821E9EE3A43CADE1B80"
              "D92A0AE8DCEB4DFF766A54DF3665FEFE3C252B72DAD7B1E3359E7FA25562C3E3"
              "9DB521CE1874111FB090DBD38B3180AD034B57B031DC4DD6AF7C1A8AF3F6CE7E"
              "DB1A9E4B6D4A5920E3620818820659762EF7A4243F51DF2D8A900737D5810569"
              "9B4E10CBCB359C7F3A4007697C482050EC33CF8041916A3B919A50D96EF0F589"
              "FD4556F30DBDD942EAB79DFA97C07E30247074352E1BF98E349CC7EFA5A1B8FC"
              "E4F18F1FAF6F07C99C321448B0395C8A9CBC466412F89C1A98BF5715842844F0"
              "E8236FA4696C4658B8FDE4425D09D67A38AC7258E5D5966F2D3FF66A0C0CE76E"
              "7F6B81A1BCD047FD3A205BF0CCAEA3B11079909C6CE5698F32E1F3409658FFA0"
              "1EAECB4AE2B092B78989DAAD6623BB11F49F0F8F8699EC05661502FFCAD03CF4"
              "15191A222D3C4C7B8AB0B5B9BBC2D9DCEFF7202D3F4244494F525364666974C4"
              "D9E6F5FA0001041927373D5A7680B8C1C9FE2029383B3C484D565F65799D9EA6"
              "A9ADD2DEE5E7F7F9000000000000000012243248"),
      },

      {
          /*TEST 2*/
          /*public_key= */ absl::HexStringToBytes(
              "B541C1E92CEADD904A09EC08AD306D974734A077868471E58D077187C46604CF"
              "2DD604D5365711DAA1AFD06E8EAF687C3999624D3C181084A07273B55EF3A84C"
              "6098703ECF97F7D464C65217AB2D5D3489353C3E2B17272455B08A92180BBC9E"
              "CB8816C54D98800134B238E01584C1077CAB47128380D92634CB291E958A62AC"
              "22F0501CFF047AB9756D58E2C46CA1CBEDC61185721FE00DAFFC5EFDFD40E2D4"
              "5F068AD18C42CDDC6C26F042FBBAF8DE524BF10C5E56A6369FCBF7414BA851D7"
              "546C0ADBA0DDDE66A9852F05B812FCCB66CA7A0ED2C5BD3655BFD8AA4BEFB863"
              "FCEEE2EB4F6681C75A75D42CA460016B98A775114CC8376C742BD7202F81E37E"
              "ABA9E4378B7BD3F0EDBA40AAB715D45E6663C99BE63438B958368CB23F42F54E"
              "BCDCDA4F8570CAFE2EA1728C0B23D9BB22FAB6922EA6694D7FDFCA08C77ED488"
              "AC5FEFDCA7889EE4ED5FDAA7A126DCA6D5EF6C43BF35B6122E6EF0E581AC3382"
              "38E514AA3AA96248EB6D5838D3417412758233A05C56EC4B5C5F1B59225E53EE"
              "2DA1F8326C50E6988BF614587F37BEA87BF457154634CCF3E7E9A6D159049251"
              "80DBE0EA7B2BDA615A4DE6E83A4847D60AC4AC7F80A93DC6BE06A4801021FE7C"
              "717858F5648A5E53870C6D42B77D29890EB545657B7AF6E140A7E14B88D31DC0"
              "72CD9A41C66BB2C7089F4F167BA0DDF908AD7A2AB0CE19CC4A177E0CF12C8AFE"
              "C9858C3924738F04DDDFDAF853F7EDCBE52FCAF3E382C712C4E821C24DF91D15"
              "73A74CE21E44EFBA8A8EC6E6D5EE0BD8EBE02436623572353E4210CB84D1283E"
              "1F6BA726B2746EB4362D464CF2E278B2C43568C9D77614DDCA1B193D3CCA67FC"
              "7B4B4117D1FE2D1780EF90538AFAEADFB376E318330E785AF103753FD59917BA"
              "BBC0C8D7532E0A54BF8E91F4AAAADC9D53513A04A1B530D711C40469063AB2FB"
              "F5EAEB2A941A58126EB5802D1A97D44BF34082B026410A83D42DFB4C5DD1DBA8"
              "C19D33DD53B61A7B586AA06DC4414BB4C53F26E05A69CB719CFDCE1E272977C5"
              "26CFC1B3A72482AA2461F1C46724F2D9CA9D31F93E9D3C55D944A56DB9470E45"
              "41E217C802C19A2F9323EFB58F62CAF6FA86061DD88A89F08271318DE8A81A56"
              "6564332010FFE4C88A2A022651AA32F573B3B154AA8CF10DFBDCC0C57B2341CF"
              "9D6E5FDA17A7A3616D2424CA2B9FE08DC6C296DFA2BE9DE8E53C328B8D66142A"
              "E233EFA30D90E91A75786756B8AB25C15B91CF34A0E12DAC9D5E525AA58D229D"
              "4E60DC2AB27C61790FAE215C200520216CBB398A209B784E31AF7E15F0A00731"
              "9574887E6C2766A04096AF1EDC4593F2FA918F1A4851EF4CDB7ADDC408DC3C57"
              "915C8BD990C10006169D84FBDF13C7097BEC77E3F147576023232A7450F7864F"
              "21B328ACBA0A4B21256B1B08D4A4CD7AA53A307BC1AE360D78E93D4382BCD583"
              "6E3019F67A1F5C39A30E8E9FA7B9622CFAD11A50F2E4F17DBE8FF2498BEFCE52"
              "8F2413142638ED76595EA25DDD6FA9AC4DF362FDC1AA83640A0339194DDE8303"
              "61A12DD3F1BB83AA7B22C4D5FC7A69FAEBEBC480EE83E80B62D7BB68FFACBCAA"
              "B2E48FE81F209BD9249151B2552FFA3E5D79FF5413C94E6FF769DEB97B2908DC"
              "DC836EC70BFFE8F7078EEB14440E0140FB22E025BC1A103267F9EA8971063472"
              "B33949AD2A15C17402513100208D726097182FA101FC38C5C4816A3ABCCF9E44"
              "BF3F2D634DA9BEC7679771491485213DD625B327D53FF4ED21E1FF19E5D6C044"
              "7F77BBEDF3F37C637A185FFC18A5EFCA4377E3486CEDF58A03DB4B023CF517E1"
              "1D80E8E293544020A4FAB7809D32CBF0151FF23CC1B2C4BD0E4107C5C0D2722"
              "E"),

          /*message = */
          absl::HexStringToBytes("225D5CE2CEAC61930A07503FB59F7C2F936A3E075481D"
                                 "A3CA299A80F8C5DF9223A073E7B90E02EBF98CA2227EB"
                                 "A38C1AB2568209E46DBA961869C6F83983B17DCD49"),
          /*signature = */
          absl::HexStringToBytes(
              "B5F89AE90773F49FB0AEFAFA2E5AC95DB65E534A431E7B641FEE751F8996C367"
              "17F3A8447C995D475BC1C3404ADF42E9FD898B54ED099AB54C5F471BC7C4BB39"
              "2530F821058DE4B2F40EA7EF2A297EC40D654467954888557D89C22F79CB44CC"
              "9311FF987A9EE26191E427E9AF8FC80FE758FD4BB1886D83B230634FD65CE53A"
              "03699EABF32920157814C97DEE6C485C7E98A4317326F5D6399D73B3855CEB17"
              "7791E339093D62E67D2C5B2E16AC2DCC0C547D7819F1C0CF3FE7346144A89E87"
              "5BA1CBC07528FF75E57C7E06DBE99658B6AF15D9173716BED3FDEF7CEA4D330C"
              "31F673373253C55A75A114BCD07F0CF523DA09DC23ADCF8F828AEBE820793941"
              "5D0B0DCEDFFF1A04A4C23BD562132920D6D3EA9F633DBF485C246C0DC76CF409"
              "80E351A88B19E9385D987AD9C584C425D35DDB4DE7956B8B12EF2BC11A5CDA22"
              "A7D338D8107C637916CD9F8FB404EB18B1B6CA08B5E9D39CD41FA8F0E166E812"
              "D2349F6A15654AB713C3EB19056E02F95B71B918C6109A3979C466290B0BCD4E"
              "D579C5084F705EF1A02107599689BEB4B3E0630EC5CD7F3CE58DF5EA6012E09E"
              "30B9DFF65D0CD55BCA59BEDBB2A453683D71A1D69992761DA4F6C2B376A87D7E"
              "7803F7C2A9927E4A560BE5F80ACA92F828C99A63D82A3AAACCC6D9BC7BC8C5F6"
              "706C0548F5110ADF4864EC6201E7B22A6C13B67F12B7A0598C98D6C27E60D481"
              "0FD5167995E66E30773BF7CB03F3539EB8E72B8384534DEB7DB72B847DD5C66D"
              "02FB4E9505B008419C722302A155957B796BE877CFDB17CB68ECFC590C6F1258"
              "3468CB454C67BE3F2E861A389E6F2065DD2E4FBC1D4CDDC3AD1C9A3116EF31F6"
              "0ED85577AA2EE2EFF7217DC0A1734E0143CCDB3464234EDEFEFA8D0561D1B2EC"
              "9AA9E78EC82FB059B3EED329D40026AB7395C42CA598D37E69729C373A07FE48"
              "A191B91FF3C962E29D0C9C40536BA7B6522012A2EE4895640F0742A6F20289AB"
              "8E6C604C1C569BBE1F6BC3449F44FD1CA251D2FF2CA469230F79129775BFC72D"
              "BA912FB7E96A7F875C90C65CA6B99D1728B792E2719516265DCF9063CD7099C7"
              "29F0425AE747F7E026BF41FFCB32EC89EFAD9144880038C5720E54FEBD973337"
              "C05D557B74112BF23187CE41DE9CED156F084393813AFB433C292EFB37F137E6"
              "006A95ADCD580E3672181BE8D30913CA87E00806BF82A7509FD257A77591F67A"
              "780F26499CC0E93CB04260001DA343C789752CC1A438398FE048F19B0D83D1AF"
              "9560B909CFFE1364156F4B4562D1575E32C77F8F0A267BE43E8372EB4A59695A"
              "625B84C2795C724AC240FC81CC1F03E01004F98220F3B49E1B896B422049D15C"
              "4E5480D2E6DAE9A7AD5E5BB4F306EAFB6AA5166DFB5ACF5E7FF0573E3542CFEA"
              "FE1EDB4F1E405DD3167A928E30DCC60EF862B5A0DBCF00278B0323EC6CBAE14C"
              "8D799BCC3110893012EA817EA06F85328748DF009B7DD36C466552C63F550AF2"
              "D2367A3C17F7866FA22F1C8F16D2CB9F9AD79FAED01E61EB31B00AFD17A98443"
              "5B1CA27AE4BBC342482F2472451118B2897E6CF750FC1F53AC8081A69866284E"
              "EF0872AC3EB3427A5155A16606BC5D6D0506DC48F1EB3E85F71FCA62D5D9D3AB"
              "E3BB3E9B03C4EEF7B269B5A85ED3CB14CDED13C1AB926941522A5BED34B2BC33"
              "C11733142223C45D505546278400EA9696A4F419CC80B13FFC3DF5E0E6354129"
              "3B51CE006AD0A51CE956FA3CF905FF131993767818278F2123F09F4221212146"
              "FD06B71DC13DC264CCC9E3DD946EBEB9B4065683818733A97754CC85BC869B69"
              "7B1B99011C32EB6EC4FF8AE3F6FC4EEAED428409C5B034257A0B96F005737D47"
              "56E77CA544B90E841F8B47EE8204EA85E3CBA914A039CCD9CCD0604F137895B0"
              "352917DA6990B01A87AB5BBEBD41207C8E9A43CA10279D7CA709D64D36CFA22E"
              "D50134DE4BA38349116492D74B239208DFD19484EABCFE399C985CD0CBCFE450"
              "25D3558EC9D380AA29B1BE2E65462093B73AF645777A192C0B471206C14FE2E4"
              "DCD6115B4C97ECB128864D2BA031F12B44F3861B4DA5714E78B4F7CC31B5C8B5"
              "04D1915E5DB89660F4CD7A5457683674BEB31C09679F30AED229CFE5EAC8F2F6"
              "18416B009B17ED3E95369ED1FBC84FD811B93BE765C43AD7E1313F7C23364CC5"
              "A5CED5259A16D699B7DC938AF8ABF2B7F7226776CDF877D5A83B2224CEDD494A"
              "407915747F9A268041B7439F1C49B88051B12D1F039DBFA7BD0D4B83A666A5D9"
              "A341866136A6F6FECFDDFA3794C52BE138C6AB66270E37F0490F397C8061BD05"
              "BC57556978C03E9A3460B4BC824D3DB7F51E3708F5062B42F1617A3339D77B03"
              "3AB63AD00EC0C7D1F07650BD1E26B4DB375EEB35AF5DC226A7424CF11915324A"
              "96295BA9CE3E94CA41169D7F93E650E100E863D2592BDB0DC03C3B125069FB24"
              "809A279AC6CE581A7C8C94B62B2E5F9A9200334E07924AE38DF3F40DB3910C35"
              "E6D5E7E954C44AC8E3BE20DB469905741302431FB975ED1EDB2615263328FF51"
              "BAAC89FDACADAB5E79E5DE54CA24C1E394325AD8023467B23A8FF7EC227E8832"
              "7B97408F8AD23AFEA5F9A81E399B9279C2DE787737ADF383B48358568BA04B41"
              "2489BF78D635C0A5DA0FEDCDC0B7AFCB88F3B835894BD03857325137A4264DBD"
              "4012926F9E8C3EC621669957413EC511CDCBB4A31F3F607D289EEBECCD86E992"
              "303166E60B8A126CEF13902D4AB177FA23B0EC0D726C5957670458E322539BF0"
              "FC193EE524DABCEA6C7433FEDD5637872376D9E4918FF551A6E0F40C1AB754DF"
              "606DE645E3E7820C853FEEC06A7D45CF879F79072C30595152F29EE3B3BA04AE"
              "33A2872980FC6DD7C231C7FA347CBD68D9B2EEF5786E48F78A8A280250609028"
              "49CAC6702583B11D37677868081E06FC6A7EDFBE6B9CBBD2C7370B2961704B05"
              "F357FB4633A9EF6B2EFEB2D8FFA31CC90FC4515A953F8F7F68819318712E0644"
              "093A0A18E431E4642876316CFC127D674B676F29C90C9D251421495417C0C1F0"
              "96BC1CC8D6BFF17F953BF2485D1950A09D3C3A9288E6CEE230CF41C34B1F3BB8"
              "330A9607CB62A9510FC25A0E5F670B48693C06F8A02C297DBBAFE56761860829"
              "E4B55DF0C7E00691B5E088CEC806780BB3AB6C2C068C4ED8FC47FE3976D1651E"
              "630ECE0FA77B5F05A6FE70209D31860CEB98FA49B7EC55251AEB7C7C9016D180"
              "405A1E5A24691551CFE6FD8E8F3617902BE0F63353E73F054601F0CD2B1CE8BF"
              "B20B7C649D31CE52C1B7EFDDE9D9D86B3952186CAF0C3CCE1FDD130426E42E02"
              "090F5B5D5E74757B82A7B4C2CFDBE41040434E626667698B989BA4B2D2E0E9F7"
              "09212A3B465258718EC0D7E1E6EDF5102832394F565778808F9BAEB2DEEF0000"
              "000000000000000000000000000000000F202F3E"),
      },

      {
          /*TEST 3*/
          /*public_key= */ absl::HexStringToBytes(
              "CF39B474CE5D8EEB353C885DBC60D2A95546F4D2A97B9F0E46C5E17C1A8CC139"
              "26C2D30AA25B6D291E580CAEC55631BFF6173040266DF8B55B1B29147F0CC405"
              "896BAF2AD7D4CE2BD83FABF53BC906EE9704B3726B532E3CF8D6A28DCBB3D65A"
              "498D7DA0652E104B37D209EA9E3BE5A29D06B61B7D7B5CE3F14CC9F34F7B5059"
              "6BD74C043AD6F54160782A34795FC7BC9B541DEA9C26095DA4DBDFB724049B31"
              "FD61AB813F032513D5C8BBD05BDDE6A6631DDBD909906CB0808EDE005FE5B45C"
              "12F83DFC8CE3F4BD4E8285AA5E73CF6364C88BC0DE69052D346CE16B4A221A5B"
              "D31E0F03F0F7FE6505808616F05720424311D42F301505F6635B0CD0842F610A"
              "CFF61BD05EA384A3C4C96517D456A4B13B2DDBE65533492116641B27E4623B25"
              "DDEB92DECEAF778EF87B2FB35EF0DF81CEB00DAB36E422051D0FBF409A89F8FE"
              "336897454DF54892AC65E7EA36EA59536A6F712602111BBCC4E47835B031698D"
              "08287CBAC0E7AC3BF93BC2DA6D2C4C19E17A68F1A4D7744F0ECD4E792E7E94AE"
              "082EDD9C07DA4602E9A400B98431F695D778FD5153A52C7AFF2CB88F4D8CB6D2"
              "132257E8B6068944C6D15B2D6040E917423BA59C00C713A548C63D88F366CE1A"
              "DE8FED54EF4343A96739FC87E280DEE3091E1DAFB709135009AB4B21DC8F80EE"
              "EBD5815CA62F3D79352F25BEC8C5457542FA9E7ADC90DD9F78AF13E5D7CBFB88"
              "B81DD9199B544364BD88E46C4E2878B2C708F1AEBC496EEBAEA281F8B4B30752"
              "F7A1A09481B6CCD8F1F78C5D4BE1DEDEDD3907D466F080DD2535D1196A15FF9B"
              "B6951B8A6D19A2902B41DA639B5C1761B2334F8B2A559940E30A3FC7AD8B23D8"
              "E5479EBCB1AD2B8E63EAAC71868121FD96A1153506A76D98BC8CA2A32E0B4DED"
              "BBC5CA590A2556E0A361ABBD36E0F81088EF59BB201D01709733F24510B6D536"
              "DA2639900E3805C5723B099A5C5C3AC9C1CE7F18136907B8CD8710B9319D833E"
              "CA6B0F38F3F09E2BF0699ED9252F121689C43AAFB64901F3EF6428675BF16D3F"
              "8FC489DFD2C7CB1A51D9AB278157AB8823EA43393232553F22EDB1446E60ED1E"
              "3CE94F3DB25BA32431EB8178008E6BD14B433C109F6CDBA996EF63078505F929"
              "7CBF7642199F8B5D3CF560677DAFD0B286BFF3A9CABC780111F9B3A2542121E5"
              "5B8C0BA9B543C4DDC9DF37772C16FC7A2F4F87194E95AD887D4FCD4D4550175A"
              "693E17B53D10F2587D3B6E00BCF9EF0D6C43F99A74D1A5F86C4D2D10CD2E6246"
              "3CFA3DB0D48AC3DD908F333FBA96178C5AC3A0A83FB009FB63207A1EF944419D"
              "A76E96480E07648E732D0F4710B381672E71E5F8DB9CF378E2BF36B74405E92C"
              "44B81A5F072EC2AB975E94546F463172822A9672DF126F7651FFCABA47F1C23F"
              "428ABC04BB060E1F53A12328E62C91EC2E46597019B2271D73FC14E0B777E7DD"
              "9E03E97A6CEB5CE2C9E5347D47ECB49A60BF15022EB86A0D1BAD4C5CE0F6530A"
              "A09E773A0C274FF32A4368AA73487423ABFA7B07330163A65B9B53A6C411EED3"
              "9E61AFEFB96748FD0430630D6C6CA61FDB9CA0E24ED2B560C59C041477263925"
              "FAE5C7D2883082FCB180D0C0F1F1D8DE66E4EFA799DDB88BA849C0F229FC2C15"
              "A45536BA46DBB8FAC487AC7C551D1DB9F0D93EEECB6972DBC46F943A9D179E86"
              "45041C1C23782652631AE6BC4B31FBA2ADB7B6273F6C077059B89662C09EBC88"
              "A32FC011A19A6F0640D6C31C8C68A3E625B7C5894B038A527C6970DCD4A64E1A"
              "7143592FCF70A6075D73555231E5F6EB86DEADC797C085103EFA53DD8EC31B4C"
              "F6A4E55E3309A24119F988FD4074CD1281644A0E93E3FD34387472C6E51AF0B"
              "D"),

          /*message = */
          absl::HexStringToBytes(
              "2B8C4B0F29363EAEE469A7E33524538AA066AE98980EAA19D1F10593203DA214"
              "3B9E9E1973F7FF0E6C6AAA3C0B900E50D003412EFE96DEECE3046D8C46BC7709"
              "228789775ABDF56AED6416C90033780CB7A4984815DA1B14660DCF34AA34BF82"
              "CEBBCF"),
          /*signature = */
          absl::HexStringToBytes(
              "008714812FBAD943533DA0072378292FD28BB526806E9E501D44AC4E299D5AA9"
              "53691F276EF4556D9E7EDE41D2219D5974325BC1D25A1E7163C7748737CFE394"
              "B50F1443B9B18000CB046368156B05C2DB3D9E1AB55EBE7A6B07AF49C6AD00F4"
              "04336614AB6F2622249C8758505A58404344CB5199A1B3E6AFF48E5D2032AB42"
              "FB57F925709E8C8D6189A486C7906D8C01991E4ADA6FBED3B85056A84BA03CC7"
              "93268DAC4DAF84F37217AF9E02C3CEA326DC0D89C37E31AB0FE85B5C567CEA1B"
              "682BFCB1F52A6B4B73ACFC61BD1D302ED87572F6A30950710E696615FF8D3A92"
              "29BFCCC020CBA50A3CE45ACCA21E2C369CE3FBB3EEE2110C157EEFD3913F4374"
              "3B8F777D8E8FB0E1226083C415A0BCFDC28E22825BD6ED2DCC2DC12D0EBC0632"
              "31F69617A6EE4C0DA49183FA59D95403C677C7526EF712357CEDB6967CB14D6B"
              "FD0B5CE0F459D21ACB07C806AA2E739A669EB09931A17B12113E080038F3931C"
              "1D6823350039318AAE2B2C01EC01965D6769C2C60675A0E02625F46399CF7814"
              "DFFBE34DE920CE303571CC781FA303D94FAAE23D51DB91D7ACBBFCC412114C22"
              "3731546DFD7214FE0A7F438245C0F0665537B58AACE1C4F492A03B3000F204BE"
              "E9F52A91194083010AD853C062820950565B0F2767174A3685DCF5321102C933"
              "F68C1EEF2952A49636F00EFEDA4C22DF0674FA5E1F05129CC1177C230E199BAE"
              "2A90BEEFD011029003A8647BBE3865DA4C423C1794C1A749889406FA50C6F4C6"
              "B1246513BC4C203A0918F9311F91ACAC81AE563AD70075610D4541338064E6FE"
              "BA2F36C68D092C523D043285BE76BEB72CA161CBE9C27CD54DE40DC6B4049F33"
              "84DB0436CD223306D86046CCDE52D9BAB8F1D16D4E52F072E20297C002E87E73"
              "4ECD914B020B2719299706C2CC43FBF51297754D1F6D045EDA823B877DB692FA"
              "6F641A59AF4991BE54608488963FBC3F746D956F88C7982A1506E934514FD41B"
              "FFFF9D5C1E6472D9056ED0593583611C56FA06D3A5AFFF2A6705FC15C6C63E76"
              "333DD6ED405D0D9812E2255B879B2FFD6698A68522F643D1E93EF3EA23C883F0"
              "B103AC6D0C68BB415189C37E15EBB084E70A0E6307E6D3A9004F6699BBF4DD23"
              "89ADBBC4BABBAB6A7BFEC79521908210BB9085A46C883784CFAF39CE42626945"
              "A52259DA09C9659ED2F1F5F1920CE612438AB608036798517A10F1EA658F6AA0"
              "66E1331B0314FE4349D0059B64103ABD5C5E8407311F1E8AEB0A9B59FB2AFAE1"
              "F4CBF7E3E3ECED579C802FAD28B9300826804108EA0A28D109136E2AD78AF75A"
              "D227C16A22C2F3CA4C9DD384722ADF82C0090BBDB194A73C0C344A81A03C1BE6"
              "2F1A26C1F4EBC2D955284C9133C70D4B5DE2C1DD07BD5533527545A640FB0275"
              "53277968D1F6537C1F98793C6EBDE44C51294F6FBB222C658423F5E925093F6E"
              "96179B38003274F7A2DA1C8ED77284D399CF9607264574581A2364D2EE71C2AA"
              "3A0FABD1EF9D29E959C2BB05FA4452921DE5A3FB22EDD47277E3AAA123372B8C"
              "879D9093B4BDE86F35D3EE677272372A0A56021F7B8EDC3C564FCAB57EEA61D8"
              "2C066D9575759C9186D4AA9CA4FC64AE2FB96D25A93D3F6B17ED513B39AE9591"
              "AC2B5D9B5C1F68B99295997F0AC6D7F61595AAEE07EBF6A0503504F84314BE45"
              "172D7F71F85268993998AD8EA0F8EC514039106B0AB03FE3A667E8B91D794E0C"
              "D2754F1B689490505117F2F582DC3EF048CD5F4ABB7C7409D5771923E2E4A41B"
              "8C5D8E7275E350C85297823DAA4A7A0E45AD1DB0F0F38595B3802DC4B0ECEB68"
              "68E8904DCA04CD5BB306C91235F3C5EA803553DE9C5753558201B9B721FEC57C"
              "1DF73C377ED81477A4C761EEC57C4E6D1C58193C8126ED87DC848A55DC7124E0"
              "4F70774AC40FDC3A7A9282651BD2D309AAA3975EEAF6606AC77048FC437B9570"
              "7404E18B8EC0DA2C9F083C573302EDA209454232FFAC50B0DF32C92EDC9D8AAC"
              "8775F2F18E6010DCD070C640CB53DC1E454DB053B40454C682F174C1355AE945"
              "F68FA4366E2CAE34FAD6ED5A322BCBEE520CDC08D4E94A15B370C72028E8F6BC"
              "6A89D0A3411E6619EF084235BC6BDA14035181D8ACAEC42C1CE330E32089B5AA"
              "EEB9996F5812CDDB96F37C3A1DA10471C5587527C22C65B3DCF001CCFF9B936B"
              "99EFA8116F7472CB000E05023D937F7188A860295F9BF864AB2521352DB0144B"
              "6C203F419B3D5A171C6E4CCA500F2C9C9F84BA8277E9AF34879388B7EB13A8F2"
              "C168F4E6CAFF024ED8B53F7257341657CF4E53486C63C5AF9945754D00BE33EA"
              "D22F1C9DDDCCE404BEB01375B3B1666BCFAEE3DCBC13B7148B78FABA4FCEDAA8"
              "F7365333A4AF885C0A0899DE6DFCCA896092247F3EC3FF2070B551C7E4751C9F"
              "0AB9A18BC9C299A43683D2126BE816856E6D92C941E8C3F0D029B13E38E2FC08"
              "22771CC3E7C576A38FF014AB8405A813D4EE9C276988AAC20429FB88A0E3B029"
              "8B3926F385C920740E18304C9EFBA237D5F03C4198B1403098DD4BA395010476"
              "0307B7629279B01A739C2A71432CA075E4947436ADBF0E53EDE28CD9BCD4F0FD"
              "C6F862D1820F84F10B55907FB42BE6711356E2466929AAE8DAB8E5CA6FC3F6DD"
              "BC6363AA58CCD17249CA94329CA1A217A2E1EFA9AAA7E1350CB5B4190ECB0122"
              "EBC708597CCD798CEE3A5E97A7B7EA632C3DFE8DE68ACC44DD7761B728018E26"
              "C2FD425E5D2E32592644CF89E9528AD0684C31FC43B5C4DECCAFEBAA64F61BCC"
              "52002692151F5A722EC4E2942CF1269F1E632BD678D7CDF5F9A6FFDF061F84A9"
              "FE1F72E597787A387BE5AB5F0D3371EE2CBF45F5BE2ED64D12824E2B2A3AD340"
              "550169381D070ED208F46B54DF3ACBFA975649EABEAB1DD3D4ABDFF94B7418AD"
              "844FB0304418A343EA35974AC87CB899DEA273D96C618C4EAF6D29BAAC0A8380"
              "DE51E356D6C9911B26EDB38A820F09CD634526E783C85B5902551387D5CB00D4"
              "FC68A92217ED3589C379CFD43D923C4A43663C443D53989C1488E185D5BF9A7A"
              "A6F8C3FD95355AD7F47880526A46831ED4CAAC2C7EC510D94513BD645511440F"
              "72D8AC30238333C45D10C7C08A94502B7D490431FC0DA1FAB45F73261219B4DA"
              "1593D57F946E5971E3158EDA6C3E1C1149403240F19E5D7C4B54D07BAA9BF30D"
              "CE7E50FC23ECFC351CF9FB5256DAC143429E62BDECF0D8AF24C2D3769E89DE72"
              "BC01510198F8F07BBF0FBCDB363969E30A4A711EAEEC98A122164D399C970977"
              "33BD224B361BDB48C0C1E5520637340B94C20DD0491CA18241B1B08429DC8D0B"
              "02090E292A3A3B4586879EADC9F4FCFD00162E3E7D9DBEC8D8E8E9FBFE365B5F"
              "75898FA4A9D6E30918363D474A5972777AAAD1E7EE0000000000000000000000"
              "00000000000000000000000000000000101D2735"),
      },
  };

  for (const TestVector &v : dilithium2_vectors) {
    // Create a new verifier.
    absl::StatusOr<std::unique_ptr<PublicKeyVerify>> verifier =
        DilithiumAvx2Verify::New(
            *DilithiumPublicKey::NewPublicKey(v.public_key));
    ASSERT_THAT(verifier.status(), IsOk());

    // Verify signature.
    Status status = (*verifier)->Verify(v.signature, v.message);
    EXPECT_THAT(status, IsOk());
  }
}

}  // namespace
}  // namespace subtle
}  // namespace tink
}  // namespace crypto