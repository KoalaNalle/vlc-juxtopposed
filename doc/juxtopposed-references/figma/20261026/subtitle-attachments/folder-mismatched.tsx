const assetPathPrefix = "https://www.figma.com/api/mcp/asset/0be6827e-df6c-4c49-bc22-01b6a73cda8e";
const imgFrame1 = `${assetPathPrefix}/1b0f7.svg`;
const imgFrame = `${assetPathPrefix}/cf6e1.svg`;
const imgFrame2 = `${assetPathPrefix}/764c9.svg`;
const imgFrame3 = `${assetPathPrefix}/d74b3.svg`;
const imgFrame4 = `${assetPathPrefix}/9c2da.svg`;
const imgFrame5 = `${assetPathPrefix}/a8689.svg`;
const imgVector77 = `${assetPathPrefix}/b0920.svg`;
const imgVector78 = `${assetPathPrefix}/a421b.svg`;
const imgVector79 = `${assetPathPrefix}/05902.svg`;
const imgVector80 = `${assetPathPrefix}/03b9d.svg`;
const imgVector81 = `${assetPathPrefix}/c3882.svg`;
const imgFrame6 = `${assetPathPrefix}/8abe2.svg`;

type IconProps = {
  className?: string;
  direction?: "default" | "down";
  eq?: "default";
  name?: "settings" | "chevron" | "check" | "folder";
  state?: "default" | "active";
  twoTone?: "no";
  value?: "default";
};

function Icon({ className, direction = "default", eq = "default", name = "settings", state = "default", twoTone = "no", value = "default" }: IconProps) {
  const isCheckAndDefaultAndActiveAndDefaultAndNo = name === "check" && direction === "default" && state === "active" && eq === "default" && twoTone === "no";
  const isCheckAndDefaultAndDefaultAndDefaultAndNo = name === "check" && direction === "default" && state === "default" && eq === "default" && twoTone === "no";
  const isChevronAndDownAndDefaultAndDefaultAndNo = name === "chevron" && direction === "down" && state === "default" && eq === "default" && twoTone === "no";
  const isDefaultAndDefaultAndNoAndIsFolderAndDefaultOrChevronAndDown = state === "default" && eq === "default" && twoTone === "no" && ((name === "folder" && direction === "default") || (name === "chevron" && direction === "down"));
  const isFolderAndDefaultAndDefaultAndDefaultAndNo = name === "folder" && direction === "default" && state === "default" && eq === "default" && twoTone === "no";
  return (
    <div className={className || "content-stretch flex items-center justify-center relative"} id={isCheckAndDefaultAndDefaultAndDefaultAndNo ? "node-494_2400" : isCheckAndDefaultAndActiveAndDefaultAndNo ? "node-494_2394" : isChevronAndDownAndDefaultAndDefaultAndNo ? "node-71_868" : isFolderAndDefaultAndDefaultAndDefaultAndNo ? "node-51_808" : "node-42_355"}>
      <div className={`relative shrink-0 ${isDefaultAndDefaultAndNoAndIsFolderAndDefaultOrChevronAndDown ? "flex items-center justify-center" : "size-[24px]"}`} id={isCheckAndDefaultAndDefaultAndDefaultAndNo ? "node-494_2401" : isCheckAndDefaultAndActiveAndDefaultAndNo ? "node-494_2395" : isChevronAndDownAndDefaultAndDefaultAndNo ? "node-71_869" : isFolderAndDefaultAndDefaultAndDefaultAndNo ? "node-51_809" : "node-44_413"}>
        {direction === "default" && eq === "default" && twoTone === "no" && ((name === "settings" && state === "default") || (name === "check" && state === "active")) && <img alt="" className="absolute block inset-0 max-w-none size-full" src={name === "check" && state === "active" && eq === "default" && twoTone === "no" ? imgFrame3 : imgFrame1} />}
        {isDefaultAndDefaultAndNoAndIsFolderAndDefaultOrChevronAndDown && (
          <div className="-scale-y-100 flex-none rotate-180">
            <div className="relative size-[24px]" data-name="Frame">
              <img alt="" className="absolute block inset-0 max-w-none size-full" src={name === "chevron" && direction === "down" && eq === "default" && twoTone === "no" ? imgFrame2 : imgFrame} />
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

type TargetProps = {
  className?: string;
  state?: "to this folder";
};

function Target({ className, state = "to this folder" }: TargetProps) {
  return (
    <button className={className || "bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] w-[200px]"} data-node-id="1959:5324">
      <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] text-left whitespace-nowrap" data-node-id="1959:5320">
        To This Folder
      </p>
      <Icon className="content-stretch flex items-center justify-center relative shrink-0" direction="down" name="chevron" />
    </button>
  );
}

export default function AddSubtitlesToPlaylistFolder() {
  return (
    <div className="backdrop-blur-[25px] bg-[rgba(14,14,14,0.95)] content-stretch flex flex-col isolate items-start relative rounded-[14px] shadow-[0px_8px_15px_0px_rgba(0,0,0,0.46)] size-full" data-node-id="3336:11929" data-name="Add Subtitles to Playlist/Folder">
      <div className="bg-[#090909] content-stretch flex flex-col gap-[12px] items-start p-[9px] relative rounded-tl-[14px] rounded-tr-[14px] shrink-0 w-full z-[3]" data-node-id="3336:11930">
        <div className="content-stretch flex items-center justify-between pl-[10px] relative shrink-0 w-full" data-node-id="3336:11931">
          <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#8f8f8f] text-[14px] whitespace-nowrap" data-node-id="3336:11932">
            Add Subtitles
          </p>
          <button className="content-stretch cursor-pointer flex items-center justify-center p-[4px] relative rounded-[4px] shrink-0 w-[32px]" data-node-id="3336:11933" data-name="Icon Wrapper">
            <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:11933;177:88" data-name="Icon">
              <div className="flex items-center justify-center relative shrink-0" data-node-id="I3336:11933;177:88;45:41">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame4} />
                  </div>
                </div>
              </div>
            </div>
          </button>
        </div>
        <div className="content-stretch flex gap-[10px] items-center relative shrink-0 w-full" data-node-id="3336:11934">
          <Target className="bg-[#151515] content-stretch cursor-pointer flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] shrink-0 w-[200px]" />
          <div className="bg-[#151515] content-stretch flex items-center justify-between pl-[12px] pr-[8px] py-[5px] relative rounded-[6px] shrink-0 w-[200px]" data-node-id="3336:11936">
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11937">
              From Computer
            </p>
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" direction="down" name="chevron" />
          </div>
          <div className="bg-[#151515] content-stretch flex flex-[1_0_0] gap-[10px] h-[34px] items-center min-w-px px-[8px] py-[5px] relative rounded-[6px]" data-node-id="3336:11939">
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" name="folder" />
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11941">
              Videos / Shows / The Wand / S01
            </p>
          </div>
        </div>
      </div>
      <div className="content-stretch flex flex-[1_0_0] items-center min-h-px relative w-full z-[2]" data-node-id="3336:11942">
        <div className="content-stretch flex flex-[1_0_0] flex-col gap-[16px] h-full items-start min-w-px overflow-clip pl-[30px] py-[30px] relative" data-node-id="3336:11943">
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11944">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11945" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11945;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11946">
              The_Wand_S01E01.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11947">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11948">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11949" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11949;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11950">
              The_Wand_S01E02.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11951">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11952">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11953" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11953;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11954">
              The_Wand_S01E03.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11955">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11956">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11957" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11957;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11958">
              The_Wand_S01E04.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11959">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11960">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11961" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11961;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11962">
              The_Wand_S01E05.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11963">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector79} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11964">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11965" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11965;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11966">
              The_Wand_S01E06.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11967">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector79} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11968">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11969" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11969;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11970">
              The_Wand_S01E07.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11971">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11972">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11973" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11973;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11974">
              The_Wand_S01E08.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11975">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11976">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11977" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11977;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11978">
              The_Wand_S01E09.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11979">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector78} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11980">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11981" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11981;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11982">
              The_Wand_S01E10.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11983">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11984">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11985" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11985;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11986">
              The_Wand_S01E11.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11987">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector80} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11988">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11989" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11989;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11990">
              The_Wand_S01E12.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11991">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11992">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11993" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11993;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11994">
              The_Wand_S01E13.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11995">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector81} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:11996">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:11997" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:11997;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:11998">
              The_Wand_S01E14.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:11999">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
          <div className="content-stretch flex gap-[14px] items-center relative shrink-0 w-full" data-node-id="3336:12000">
            <div className="content-stretch flex items-center justify-center relative shrink-0 size-[24px]" data-node-id="3336:12001" data-name="Icon">
              <div className="-translate-x-1/2 -translate-y-1/2 absolute flex items-center justify-center left-1/2 size-[24px] top-1/2" data-node-id="I3336:12001;999:4811">
                <div className="-scale-y-100 flex-none rotate-180">
                  <div className="relative size-[24px]" data-name="Frame">
                    <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame5} />
                  </div>
                </div>
              </div>
            </div>
            <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="3336:12002">
              The_Wand_S01E15.mp4
            </p>
            <div className="flex-[1_0_0] h-0 min-w-px relative" data-node-id="3336:12003">
              <div className="absolute inset-[-0.5px_0]">
                <img alt="" className="block max-w-none size-full" src={imgVector77} />
              </div>
            </div>
          </div>
        </div>
        <div className="content-stretch flex flex-[1_0_0] flex-col gap-[4px] h-full items-start min-w-px overflow-clip px-[20px] py-[22px] relative" data-node-id="3336:12004">
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12005" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12005;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12005;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12005;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12005;1915:8386">
                The_Wand_S01E01.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12006" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12006;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12006;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12006;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12006;1915:8386">
                The_Wand_S01E02.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12007" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12007;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12007;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12007;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12007;1915:8386">
                The_Wand_S01E03.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12008" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12008;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12008;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12008;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12008;1915:8386">
                The_Wand_S01E04.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12009" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12009;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12009;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12009;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12009;1915:8386">
                The_Wand_S01E05.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12010" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12010;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12010;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12010;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12010;1915:8386">
                The_Wand_S01E06.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12011" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12011;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12011;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12011;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12011;1915:8386">
                The_Wand_S01E07.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12012" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12012;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12012;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12012;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12012;1915:8386">
                The_Wand_S01E08.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12013" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12013;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12013;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12013;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12013;1915:8386">
                The_Wand_S01E09.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12014" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12014;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12014;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12014;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12014;1915:8386">
                The_Wand_S01E10.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12015" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12015;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12015;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12015;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12015;1915:8386">
                The_Wand_S01E11.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12016" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12016;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12016;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12016;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12016;1915:8386">
                The_Wand_S01E12.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12017" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12017;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12017;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12017;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12017;1915:8386">
                The_Wand_S01E13.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12018" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12018;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12018;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12018;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12018;1915:8386">
                The_Wand_S01E14.srt
              </p>
            </div>
          </div>
          <div className="content-stretch flex h-[36px] items-center justify-between px-[8px] relative rounded-[6px] shrink-0 w-full" data-node-id="3336:12019" data-name="subtitle multiple">
            <div className="content-stretch flex gap-[14px] items-center relative shrink-0" data-node-id="I3336:12019;1915:8384">
              <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="I3336:12019;1915:8385" data-name="Icon">
                <div className="relative shrink-0 size-[24px]" data-node-id="I3336:12019;1915:8385;1911:8289" data-name="Frame">
                  <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame6} />
                </div>
              </div>
              <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#969696] text-[14px] whitespace-nowrap" data-node-id="I3336:12019;1915:8386">
                The_Wand_S01E15.srt
              </p>
            </div>
          </div>
        </div>
      </div>
      <div className="content-stretch flex items-center justify-between p-[20px] relative shrink-0 w-full z-[1]" data-node-id="3336:12020">
        <div className="bg-[#1b1b1b] content-stretch flex h-[32px] items-center justify-center px-[8px] py-[4px] relative rounded-[6px] shrink-0 w-[100px]" data-node-id="3336:12021">
          <p className="[word-break:break-word] font-['Inter:Semi_Bold'] font-semibold leading-[normal] not-italic relative shrink-0 text-[#8f8f8f] text-[14px] whitespace-nowrap" data-node-id="3336:12022">
            Cancel
          </p>
        </div>
        <button className="bg-[#371f06] content-stretch cursor-pointer flex h-[32px] items-center justify-center px-[8px] py-[4px] relative rounded-[6px] shrink-0 w-[100px]" data-node-id="3336:12023">
          <p className="[word-break:break-word] font-['Inter:Semi_Bold'] font-semibold leading-[normal] not-italic relative shrink-0 text-[#d37918] text-[14px] text-left whitespace-nowrap" data-node-id="3336:12024">
            Save
          </p>
        </button>
      </div>
    </div>
  );
}